curl -X GET http://localhost:5050/get_result \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "uuid=7bc2e395-b58e-45c9-90f4-b9e5b5e671bd" \
     -d "request_id=132" \
     -i 

# {"products":[{"carbs":47,"grams":200,"name":"Macaroni Salad"}]}