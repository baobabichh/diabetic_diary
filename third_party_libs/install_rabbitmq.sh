current_dir=$(pwd)

sudo apt-get install -y libboost-all-dev libssl-dev cmake

git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
cd AMQP-CPP
mkdir build && cd build
cmake .. -DAMQP-CPP_BUILD_SHARED=ON -DAMQP-CPP_LINUX_TCP=ON
make
sudo make install

cd $current_dir
curl -fsSL https://packages.erlang-solutions.com/ubuntu/erlang_solutions.asc | sudo gpg --dearmor -o /usr/share/keyrings/erlang-solutions-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/erlang-solutions-keyring.gpg] https://packages.erlang-solutions.com/ubuntu $(lsb_release -cs) contrib" | sudo tee /etc/apt/sources.list.d/erlang-solutions.list

sudo apt update -y
sudo apt install -y erlang

cd $current_dir
curl -s https://packagecloud.io/install/repositories/rabbitmq/rabbitmq-server/script.deb.sh | sudo bash

sudo apt install -y rabbitmq-server
sudo systemctl start rabbitmq-server
sudo systemctl enable rabbitmq-server
sudo systemctl status rabbitmq-server