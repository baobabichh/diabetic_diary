CREATE TABLE Records
(
    ID BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    UserID BIGINT UNSIGNED,
    FoodRecognitionID BIGINT UNSIGNED NULL,
    Insulin FLOAT,
    Carbohydrates FLOAT,
    TimeCoefficient FLOAT,
    SportCoefficient FLOAT,
    PersonalCoefficient FLOAT,
    CreateTS TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (UserID) REFERENCES Users(ID),
    FOREIGN KEY (FoodRecognitionID) REFERENCES FoodRecognitions(ID)
);