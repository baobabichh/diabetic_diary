curl -X POST http://localhost:5050/recognize_food \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "uuid=7bc2e395-b58e-45c9-90f4-b9e5b5e671bd" \
     --data-urlencode "base64_string@1.txt" \
     -d "mime_type=image/jpg" \
     -i 