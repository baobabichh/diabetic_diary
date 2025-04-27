curl -X POST http://localhost:5050/add_record \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "uuid=7bc2e395-b58e-45c9-90f4-b9e5b5e671bd" \
     -d "time_coefficient=1.0" \
     -d "sport_coefficient=0.5" \
     -d "personal_coefficient=1.0" \
     -d "insulin=10.0" \
     -d "carbohydrates=40.0" \
     -d "request_id=NULL" \
     -i 