current_dir=$(pwd)

sudo apt-get update -y
sudo apt-get upgrade -y

sudo apt-get install -y libboost-chrono-dev
sudo apt-get install -y libboost-all-dev

sudo apt-get update -y
sudo apt-get upgrade -y

cd $current_dir
git clone https://github.com/alanxz/rabbitmq-c.git
cd rabbitmq-c
mkdir build 
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install

cd $current_dir
git clone https://github.com/alanxz/SimpleAmqpClient.git
cd SimpleAmqpClient
mkdir build 
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo ldconfig

# Install
sudo make install

cd $current_dir
sudo apt install -y rabbitmq-server
sudo systemctl start rabbitmq-server
sudo systemctl enable rabbitmq-server
sudo systemctl status rabbitmq-server
sudo rabbitmq-plugins enable rabbitmq_management
sudo systemctl restart rabbitmq-server
sudo rabbitmqctl add_user user pass
sudo rabbitmqctl set_user_tags user administrator
sudo rabbitmqctl set_permissions -p / user ".*" ".*" ".*"