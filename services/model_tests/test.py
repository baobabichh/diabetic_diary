import os
import json
import base64
from pathlib import Path
from openai import OpenAI
import google.generativeai as genai
import numpy as np
from typing import Dict, List, Any

prompt ="""Analyze the food items in this image and provide a detailed nutritional report in JSON format. Split the products as much as you can. For each visible food item:
1. Identify the name of the food
2. Estimate the portion size in grams
3. Calculate the carbohydrate content in grams

Format your response exactly as follows:
{
"products": [
    {
    "name": "Product Name",
    "grams": estimated_weight_in_grams,
    "carbs": carbohydrate_content_in_grams
    },
    ...additional products as needed
]
}

Be as accurate and detailed as possible with your estimations. If multiple food items are present, include each as a separate entry in the products array.

Example output:
{
"products": [
    {"name": "Buckwheat", "grams": 150, "carbs": 164},
    {"name": "Chicken Breast", "grams": 200, "carbs": 330},
    {"name": "Cheese", "grams": 50, "carbs": 200}
]
}"""

def get_value_from_cfg_file(file_path, key):
    with open(file_path, 'r') as file:
        data = json.load(file)
    api_key = data.get(key)
    if api_key is None:
        raise KeyError("API key not found in the JSON file.")
    
    return api_key

OPENAI_API_KEY = get_value_from_cfg_file("../../credentials.json", "openai_api_key")
GEMINI_API_KEY = get_value_from_cfg_file("../../credentials.json", "gemini_api_key")

# Инициализация клиентов API
openai_client = OpenAI(api_key=OPENAI_API_KEY)
genai.configure(api_key=GEMINI_API_KEY)

def encode_image_to_base64(image_path: str) -> str:
    """Кодирует изображение в base64."""
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

def get_openai_analysis(image_path: str) -> Dict[str, Any]:
    """Отправляет запрос в OpenAI и получает анализ пищи на фото."""
    # Кодируем изображение
    base64_image = encode_image_to_base64(image_path)
    
    # Формируем запрос к OpenAI
    response = openai_client.chat.completions.create(
        model="gpt-4o",  # Используем модель с поддержкой изображений
        temperature=0,  # Установлена температура 0 для более детерминированных ответов
        messages=[
            {
                "role": "user",
                "content": [
                    {"type": "text", "text": prompt},
                    {
                        "type": "image_url",
                        "image_url": {
                            "url": f"data:image/jpeg;base64,{base64_image}"
                        }
                    }
                ]
            }
        ],
        max_tokens=1000,
        response_format={"type": "json_object"}
    )
    
    # Парсим и возвращаем результат
    try:
        result = json.loads(response.choices[0].message.content)
        return result
    except json.JSONDecodeError:
        print(f"OpenAI вернул неверный JSON для {image_path}")
        return {"products": []}

def get_gemini_analysis(image_path: str) -> Dict[str, Any]:
    """Отправляет запрос в Gemini и получает анализ пищи на фото."""
    # Загружаем изображение для Gemini
    with open(image_path, "rb") as image_file:
        image_data = image_file.read()
    
    # Настраиваем модель Gemini
    model = genai.GenerativeModel(
        model_name="gemini-1.5-flash",
        generation_config={
            "response_mime_type": "application/json",
            "temperature": 0  # Установлена температура 0 для более детерминированных ответов
        }
    )
    
    # Формируем запрос
    
    # Получаем ответ
    response = model.generate_content([prompt, {"mime_type": "image/jpeg", "data": image_data}])
    
    # Парсим и возвращаем результат
    try:
        result = json.loads(response.text)
        return result
    except json.JSONDecodeError:
        print(f"Gemini вернул неверный JSON для {image_path}")
        return {"products": []}

def calculate_total_carbs(products: List[Dict[str, Any]]) -> int:
    """Вычисляет общее количество углеводов в продуктах."""
    total_carbs = 0
    for product in products:
        carbs = product.get("carbs", 0)
        if isinstance(carbs, (int, float)):  # Проверяем, что углеводы - число
            total_carbs += carbs
    return total_carbs

def calculate_accuracy(true_carbs: int, predicted_carbs: int) -> float:
    """Вычисляет точность предсказания."""
    if true_carbs == 0:
        # Избегаем деления на ноль
        return 100.0 if predicted_carbs == 0 else 0.0
    
    # Вычисляем процент точности (100% минус процент ошибки)
    error_percentage = abs(predicted_carbs - true_carbs) / true_carbs * 100
    accuracy = max(0, 100 - error_percentage)
    return accuracy

def main():
    # Создаем папку для результатов, если её нет
    results_dir = Path("results")
    results_dir.mkdir(exist_ok=True)
    
    # Находим все изображения в текущей директории
    folder = "dataset"

# Получаем список файлов из указанной папки
    image_files = sorted(
        [f for f in os.listdir(folder) if f.endswith('.jpg') and f[:-4].isdigit()]
    )

    # Добавляем к именам файлов путь к папке
    image_files = [os.path.join(folder, f) for f in image_files]
    
    # Для хранения метрик точности
    openai_accuracies = []
    gemini_accuracies = []
    
    # Обрабатываем каждое изображение
    for img_file in image_files:
        img_id = img_file.split('.')[0]  # Получаем номер изображения
        img_path = img_file
        txt_path = f"{img_id}.txt"
        json_path = f"{img_id}.json"
        
        print(f"Обработка изображения {img_file}...")
        
        # Получаем истинное количество углеводов из txt файла
        try:
            with open(txt_path, 'r') as f:
                true_carbs = int(f.read().strip())
        except (FileNotFoundError, ValueError):
            print(f"Не удалось прочитать файл {txt_path}, пропуск изображения")
            continue
        
        print(f"OpenAI")
        # Получаем анализ от OpenAI
        openai_result = get_openai_analysis(img_path)
        openai_carbs = calculate_total_carbs(openai_result.get("products", []))
        openai_accuracy = calculate_accuracy(true_carbs, openai_carbs)
        openai_accuracies.append(openai_accuracy)
        
        # Получаем анализ от Gemini
        print(f"Gemini")
        gemini_result = get_gemini_analysis(img_path)
        gemini_carbs = calculate_total_carbs(gemini_result.get("products", []))
        gemini_accuracy = calculate_accuracy(true_carbs, gemini_carbs)
        gemini_accuracies.append(gemini_accuracy)
        
        # Сохраняем результаты в json файл
        results = {
            "true_carbs": true_carbs,
            "openai": {
                "result": openai_result,
                "total_carbs": openai_carbs,
                "accuracy": openai_accuracy
            },
            "gemini": {
                "result": gemini_result,
                "total_carbs": gemini_carbs,
                "accuracy": gemini_accuracy
            }
        }
        
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(results, f, ensure_ascii=False, indent=2)
        
        print(f"Результаты для {img_file} сохранены в {json_path}")
    
    # Вычисляем среднюю точность моделей
    if openai_accuracies and gemini_accuracies:
        avg_openai_accuracy = np.mean(openai_accuracies)
        avg_gemini_accuracy = np.mean(gemini_accuracies)
        
        # Сохраняем сводный отчет
        summary = {
            "total_images": len(image_files),
            "processed_images": len(openai_accuracies),
            "average_accuracy": {
                "openai": avg_openai_accuracy,
                "gemini": avg_gemini_accuracy
            },
            "accuracy_by_image": {
                image_files[i].split('.')[0]: {
                    "openai": openai_accuracies[i],
                    "gemini": gemini_accuracies[i]
                } for i in range(len(openai_accuracies))
            }
        }
        
        with open("accuracy_report.json", 'w', encoding='utf-8') as f:
            json.dump(summary, f, ensure_ascii=False, indent=2)
        
        print("\nСредняя точность моделей:")
        print(f"OpenAI: {avg_openai_accuracy:.2f}%")
        print(f"Gemini: {avg_gemini_accuracy:.2f}%")
        print("\nДетальный отчет сохранен в accuracy_report.json")

if __name__ == "__main__":
    main()