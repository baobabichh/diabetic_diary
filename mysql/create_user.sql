USE mysql;

-- Grant all privileges on the specific database to the user
GRANT ALL PRIVILEGES ON dd.* TO 'app_user'@'%';

-- Apply changes
FLUSH PRIVILEGES;