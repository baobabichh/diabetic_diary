curl -X GET http://localhost:5050/get_records_by_ids \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "uuid=7bc2e395-b58e-45c9-90f4-b9e5b5e671bd" \
     -d "ids=6,5,4,3,2,1" \
     -i 