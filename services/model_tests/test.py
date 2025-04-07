import os
import csv
import time
import json
import base64
import requests
import pandas as pd
from pathlib import Path
from dotenv import load_dotenv
import anthropic
import google.generativeai as genai
from openai import OpenAI

def get_value_from_cfg_file(file_path, key):
    with open(file_path, 'r') as file:
        data = json.load(file)
    api_key = data.get(key)
    if api_key is None:
        raise KeyError("API key not found in the JSON file.")
    
    return api_key

# Initialize API clients
openai_client = OpenAI(api_key=get_value_from_cfg_file("../../credentials.json", "openai_api_key"))
anthropic_client = anthropic.Anthropic(api_key=get_value_from_cfg_file("../../credentials.json", "claude_api_key"))
genai.configure(api_key=get_value_from_cfg_file("../../credentials.json", "gemini_api_key"))

def encode_image(image_path):
    """Encode image to base64 for API requests"""
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

def get_carbs_from_txt(txt_path):
    """Read carbohydrate value from text file"""
    with open(txt_path, 'r') as file:
        return file.read().strip()

def query_openai(image_path, carbs_value):
    """Query OpenAI's API with the image and prompt"""
    base64_image = encode_image(image_path)
    
    prompt = f"""
    Analyze this food image and provide the following information:
    1. List all food products visible in the image
    2. Estimate the amount and total carbohydrates in grams for each product
    
    Format your response as JSON with this structure:
    {{
        "products": [
            {{"name": "product name", "grams": estimated_grams, "carbs": estimated_carbs}}
        ]
    }}
    """
    
    try:
        response = openai_client.chat.completions.create(
            model="gpt-4o-mini",
            messages=[
                {
                    "role": "user",
                    "content": [
                        {"type": "text", "text": prompt},
                        {"type": "image_url", "image_url": {"url": f"data:image/jpeg;base64,{base64_image}"}}
                    ]
                }
            ],
            response_format={
            "type": "json_object"
            },
            max_tokens=2002
        )
        return parse_json_response(response.choices[0].message.content, "OpenAI")
    except Exception as e:
        print(f"OpenAI API error: {e}")
        return {"products": [], "error": str(e)}

def query_anthropic(image_path, carbs_value):
    """Query Claude API with the image and prompt"""
    base64_image = encode_image(image_path)
    
    prompt = f"""
    <image>data:image/jpeg;base64,{base64_image}</image>
    
    Analyze this food image and provide the following information:
    1. List all food products visible in the image
    2. Estimate the amount and total carbohydrates in grams for each product
    
    Format your response as JSON with this structure:
    {{
        "products": [
            {{"name": "product name", "grams": estimated_grams, "carbs": estimated_carbs}}
        ]
    }}
    """
    
    try:
        response = anthropic_client.messages.create(
            model="claude-3-haiku-20240307",
            max_tokens=2002,
            messages=[
                {"role": "user", "content": prompt}
            ]
        )
        return parse_json_response(response.content[0].text, "Claude")
    except Exception as e:
        print(f"Claude API error: {e}")
        return {"products": [], "error": str(e)}

def query_gemini(image_path, carbs_value):
    """Query Gemini API with the image and prompt"""
    try:
        with open(image_path, "rb") as img_file:
            image_data = img_file.read()
        
        model = genai.GenerativeModel('gemini-2.0-flash-lite')
        
        prompt = f"""
        Analyze this food image and provide the following information:
        1. List all food products visible in the image
        2. Estimate the amount and total carbohydrates in grams for each product
        
        Format your response as JSON with this structure:
        {{
            "products": [
                {{"name": "product name", "grams": estimated_grams, "carbs": estimated_carbs}}
            ]
        }}
        """
        
        response = model.generate_content([prompt, {"mime_type": "image/jpeg", "data": image_data}])
        return parse_json_response(response.text, "Gemini")
    except Exception as e:
        print(f"Gemini API error: {e}")
        return {"products": [], "error": str(e)}

def parse_json_response(response_text, api_name):
    """Extract JSON from response text and handle potential parsing issues"""
    try:
        # Look for JSON structure in the response
        json_start = response_text.find('{')
        json_end = response_text.rfind('}') + 1
        
        if json_start >= 0 and json_end > json_start:
            json_text = response_text[json_start:json_end]
            data = json.loads(json_text)
            
            # Validate structure
            if "products" not in data:
                data["products"] = []
            return data
        else:
            print(f"No valid JSON found in {api_name} response")
            return {"products": [], "error": "No valid JSON in response"}
    except json.JSONDecodeError as e:
        print(f"JSON parsing error with {api_name} response: {e}")
        print(f"Response text: {response_text}")
        return {"products": [], "error": f"JSON parsing error: {str(e)}"}

def process_files(image_folder, output_folder):
    """Process all image and text files and generate outputs"""
    os.makedirs(output_folder, exist_ok=True)
    
    all_results = []
    image_files = [f for f in os.listdir(image_folder) if f.endswith(('.jpg', '.jpeg', '.png'))]
    
    for img_file in image_files:
        base_name = os.path.splitext(img_file)[0]
        txt_file = f"{base_name}.txt"
        img_path = os.path.join(image_folder, img_file)
        txt_path = os.path.join(image_folder, txt_file)
        
        if not os.path.exists(txt_path):
            print(f"Warning: No matching text file for {img_file}")
            continue
        
        print(f"Processing {img_file}...")
        carbs_value = get_carbs_from_txt(txt_path)
        
        # Query each API
        openai_result = query_openai(img_path, carbs_value)
        time.sleep(1)  # Rate limiting
        
        claude_result = query_anthropic(img_path, carbs_value)
        time.sleep(1)  # Rate limiting
        
        gemini_result = query_gemini(img_path, carbs_value)
        time.sleep(1)  # Rate limiting
        
        # Collect results for each product from each API
        file_results = {
            "file_name": img_file,
            "total_carbs": carbs_value
        }
        
        for api_name, api_result in [
            ("OpenAI", openai_result), 
            ("Claude", claude_result), 
            ("Gemini", gemini_result)
        ]:
            products_data = []
            if "products" in api_result and api_result["products"]:
                for product in api_result["products"]:
                    products_data.append({
                        "name": product.get("name", "Unknown"),
                        "grams": product.get("grams", 0),
                        "carbs": product.get("carbs", 0)
                    })
            
            file_results[f"{api_name}_products"] = products_data
        
        all_results.append(file_results)
        
        # Generate individual files for this image
        generate_individual_csv(file_results, os.path.join(output_folder, f"{base_name}_analysis.csv"))
        generate_individual_html(file_results, os.path.join(output_folder, f"{base_name}_analysis.html"))

        time.sleep(60)  # Rate limiting
    
    # Generate summary files after all images are processed
    generate_summary_csv(all_results, os.path.join(output_folder, "summary_analysis.csv"))
    generate_summary_html(all_results, os.path.join(output_folder, "summary_analysis.html"))
    
    print(f"Processing complete. Results saved to {output_folder}")

def generate_individual_csv(result, output_path):
    """Generate CSV file for an individual image"""
    csv_rows = []
    
    img_file = result["file_name"]
    total_carbs = result["total_carbs"]
    
    # Add rows for each API's products
    for api_name in ["OpenAI", "Claude", "Gemini"]:
        products_key = f"{api_name}_products"
        
        if not result.get(products_key):
            # Add a row indicating no products found
            csv_rows.append({
                "Image": img_file,
                "Total Carbs": total_carbs,
                "API": api_name,
                "Product": "No products identified",
                "Grams": 0,
                "Carbs": 0
            })
            continue
            
        for product in result[products_key]:
            csv_rows.append({
                "Image": img_file,
                "Total Carbs": total_carbs,
                "API": api_name,
                "Product": product["name"],
                "Grams": product["grams"],
                "Carbs": product["carbs"]
            })
    
    # Write to CSV
    with open(output_path, 'w', newline='', encoding='utf-8') as csvfile:
        fieldnames = ["Image", "Total Carbs", "API", "Product", "Grams", "Carbs"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(csv_rows)

def generate_individual_html(result, output_path):
    """Generate HTML report for an individual image"""
    img_file = result["file_name"]
    total_carbs = result["total_carbs"]
    
    html_content = """
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Food Analysis Results</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; }
            h1, h2, h3 { color: #333; }
            .image-container { margin-bottom: 30px; border: 1px solid #ddd; padding: 15px; border-radius: 5px; }
            .api-section { margin-bottom: 20px; }
            table { border-collapse: collapse; width: 100%; margin-bottom: 15px; }
            th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }
            th { background-color: #f2f2f2; }
            .summary { background-color: #f9f9f9; padding: 10px; border-radius: 5px; margin-top: 10px; }
        </style>
    </head>
    <body>
        <h1>Food Analysis Result: """ + img_file + """</h1>
    """
    
    html_content += f"""
    <div class="image-container">
        <h2>Image: {img_file}</h2>
        <p>Total Carbohydrates: {total_carbs}g</p>
        
        <div class="api-results">
    """
    
    for api_name in ["OpenAI", "Claude", "Gemini"]:
        products_key = f"{api_name}_products"
        
        html_content += f"""
            <div class="api-section">
                <h3>{api_name} Analysis</h3>
        """
        
        if not result.get(products_key):
            html_content += "<p>No products identified or error occurred.</p>"
        else:
            html_content += """
                <table>
                    <tr>
                        <th>Product</th>
                        <th>Estimated Amount (g)</th>
                        <th>Estimated Carbs (g)</th>
                    </tr>
            """
            
            total_api_carbs = 0
            for product in result[products_key]:
                product_name = product["name"]
                product_grams = product["grams"]
                product_carbs = product["carbs"]
                total_api_carbs += float(product_carbs) if isinstance(product_carbs, (int, float)) else 0
                
                html_content += f"""
                    <tr>
                        <td>{product_name}</td>
                        <td>{product_grams}</td>
                        <td>{product_carbs}</td>
                    </tr>
                """
            
            html_content += """
                </table>
            """
            
            html_content += f"""
                <div class="summary">
                    <p>Total Estimated Carbs: {total_api_carbs}g</p>
                    <p>Difference from Actual: {float(total_api_carbs) - float(total_carbs)}g</p>
                </div>
            """
        
        html_content += """
            </div>
        """
    
    html_content += """
        </div>
    </div>
    </body>
    </html>
    """
    
    with open(output_path, 'w', encoding='utf-8') as file:
        file.write(html_content)

def generate_summary_csv(results, output_path):
    """Generate a summary CSV file with all results"""
    csv_rows = []
    
    for item in results:
        img_file = item["file_name"]
        total_carbs = item["total_carbs"]
        
        # Add rows for each API's products
        for api_name in ["OpenAI", "Claude", "Gemini"]:
            products_key = f"{api_name}_products"
            
            # Calculate total estimated carbs for this API
            total_estimated_carbs = 0
            if item.get(products_key):
                for product in item[products_key]:
                    product_carbs = product.get("carbs", 0)
                    total_estimated_carbs += float(product_carbs) if isinstance(product_carbs, (int, float)) else 0
            
            # Calculate accuracy as percentage
            actual_carbs = float(total_carbs)
            if actual_carbs > 0:
                accuracy = (1 - abs(total_estimated_carbs - actual_carbs) / actual_carbs) * 100
                # Limit accuracy to 0-100 range
                accuracy = max(0, min(100, accuracy))
            else:
                accuracy = 0 if total_estimated_carbs > 0 else 100
                
            csv_rows.append({
                "Image": img_file,
                "Actual Carbs": total_carbs,
                "API": api_name,
                "Estimated Carbs": total_estimated_carbs,
                "Difference": total_estimated_carbs - float(total_carbs),
                "Accuracy (%)": round(accuracy, 2)
            })
    
    # Write to CSV
    with open(output_path, 'w', newline='', encoding='utf-8') as csvfile:
        fieldnames = ["Image", "Actual Carbs", "API", "Estimated Carbs", "Difference", "Accuracy (%)"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(csv_rows)

def generate_summary_html(results, output_path):
    """Generate a summary HTML report for all images"""
    html_content = """
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Food Analysis Summary</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; }
            h1, h2, h3 { color: #333; }
            .summary-container { margin-bottom: 30px; }
            table { border-collapse: collapse; width: 100%; margin-bottom: 15px; }
            th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }
            th { background-color: #f2f2f2; }
            .api-summary { margin-top: 30px; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
            .accuracy-high { color: green; }
            .accuracy-medium { color: orange; }
            .accuracy-low { color: red; }
        </style>
    </head>
    <body>
        <h1>Food Analysis Summary</h1>
        
        <div class="summary-container">
            <h2>Results by Image</h2>
            <table>
                <tr>
                    <th>Image</th>
                    <th>Actual Carbs (g)</th>
                    <th>API</th>
                    <th>Estimated Carbs (g)</th>
                    <th>Difference (g)</th>
                    <th>Accuracy (%)</th>
                </tr>
    """
    
    # Collect data for API summaries
    api_data = {"OpenAI": [], "Claude": [], "Gemini": []}
    
    # Add rows for each image and API
    for item in results:
        img_file = item["file_name"]
        total_carbs = float(item["total_carbs"])
        
        for api_name in ["OpenAI", "Claude", "Gemini"]:
            products_key = f"{api_name}_products"
            
            # Calculate total estimated carbs for this API
            total_estimated_carbs = 0
            if item.get(products_key):
                for product in item[products_key]:
                    product_carbs = product.get("carbs", 0)
                    total_estimated_carbs += float(product_carbs) if isinstance(product_carbs, (int, float)) else 0
            
            # Calculate accuracy
            if total_carbs > 0:
                accuracy = (1 - abs(total_estimated_carbs - total_carbs) / total_carbs) * 100
                # Limit accuracy to 0-100 range
                accuracy = max(0, min(100, accuracy))
            else:
                accuracy = 0 if total_estimated_carbs > 0 else 100
            
            # Store data for API summary
            api_data[api_name].append({
                "image": img_file,
                "actual": total_carbs,
                "estimated": total_estimated_carbs,
                "accuracy": accuracy
            })
            
            # Determine accuracy class
            accuracy_class = ""
            if accuracy >= 80:
                accuracy_class = "accuracy-high"
            elif accuracy >= 50:
                accuracy_class = "accuracy-medium"
            else:
                accuracy_class = "accuracy-low"
            
            html_content += f"""
                <tr>
                    <td>{img_file}</td>
                    <td>{total_carbs}</td>
                    <td>{api_name}</td>
                    <td>{total_estimated_carbs:.1f}</td>
                    <td>{(total_estimated_carbs - total_carbs):.1f}</td>
                    <td class="{accuracy_class}">{accuracy:.1f}%</td>
                </tr>
            """
    
    html_content += """
            </table>
        </div>
        
        <div class="api-summary">
            <h2>API Performance Summary</h2>
    """
    
    # Calculate and add summary for each API
    for api_name, api_results in api_data.items():
        if not api_results:
            continue
            
        total_accuracy = sum(item["accuracy"] for item in api_results)
        avg_accuracy = total_accuracy / len(api_results) if api_results else 0
        
        # Count images in accuracy ranges
        high_accuracy = sum(1 for item in api_results if item["accuracy"] >= 80)
        medium_accuracy = sum(1 for item in api_results if 50 <= item["accuracy"] < 80)
        low_accuracy = sum(1 for item in api_results if item["accuracy"] < 50)
        
        html_content += f"""
            <div class="api-section">
                <h3>{api_name}</h3>
                <p>Average Accuracy: <span class="{'accuracy-high' if avg_accuracy >= 80 else 'accuracy-medium' if avg_accuracy >= 50 else 'accuracy-low'}">{avg_accuracy:.1f}%</span></p>
                <p>High Accuracy Results (80%+): {high_accuracy}</p>
                <p>Medium Accuracy Results (50-80%): {medium_accuracy}</p>
                <p>Low Accuracy Results (<50%): {low_accuracy}</p>
            </div>
        """
    
    # Add comparative analysis
    avg_accuracies = {api: sum(item["accuracy"] for item in results) / len(results) if results else 0 
                     for api, results in api_data.items()}
    
    best_api = max(avg_accuracies.items(), key=lambda x: x[1]) if avg_accuracies else (None, 0)
    
    html_content += f"""
            <div class="comparative-analysis">
                <h3>Comparative Analysis</h3>
                <p>Best Performing API: <strong>{best_api[0]}</strong> with average accuracy of <span class="{'accuracy-high' if best_api[1] >= 80 else 'accuracy-medium' if best_api[1] >= 50 else 'accuracy-low'}">{best_api[1]:.1f}%</span></p>
                <p>API Ranking by Average Accuracy:</p>
                <ol>
    """
    
    # Sort APIs by accuracy
    sorted_apis = sorted(avg_accuracies.items(), key=lambda x: x[1], reverse=True)
    
    for api, accuracy in sorted_apis:
        html_content += f"""
                    <li>{api}: <span class="{'accuracy-high' if accuracy >= 80 else 'accuracy-medium' if accuracy >= 50 else 'accuracy-low'}">{accuracy:.1f}%</span></li>
        """
    
    html_content += """
                </ol>
            </div>
        </div>
    </body>
    </html>
    """
    
    with open(output_path, 'w', encoding='utf-8') as file:
        file.write(html_content)

if __name__ == "__main__":
    image_folder = "dataset"
    output_folder = "results"
    
    process_files(image_folder, output_folder)