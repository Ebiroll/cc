## Chicken chase

Is now setup to run in codespaces dev container.

   git clone ssh://github.com/Ebiroll/cc.git

https://docs.github.com/en/codespaces/overview



## Changes
Removed       "open-easyrtc": "^2.0.20", support

Added "type": "module", to package.json


## INSTALL node & mongodb 

# Ubuntu

sudo apt update && sudo apt upgrade -y

sudo apt install nodejs npm -y

# Ubuntu on MongoDB

   wget -qO - https://www.mongodb.org/static/pgp/server-5.0.asc | sudo gpg --dearmor -o /usr/share/keyrings/mongodb-archive-keyring.gpg

   echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb-archive-keyring.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/5.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-5.0.list

   sudo apt update

   sudo apt install mongodb-org -y

Start mongo manually,

   sudo mongod --dbpath /var/lib/mongodb
Test connection with,

# mongo (The MongoDB Shell)
Legacy Shell: mongo is the original MongoDB shell that has been part of MongoDB distributions for many years. It is written in JavaScript and provides a powerful interface to MongoDB databases for running queries, manipulating data, and performing administrative tasks.
JavaScript Engine: It uses the Mozilla SpiderMonkey JavaScript engine.
Deprecated: Available from v3.6 and deprecated from  v4.4

# mongosh (The New MongoDB Shell)
Modern Shell: mongosh is the modern, next-generation MongoDB Shell that aims to improve user experience with enhanced usability features and a more intuitive scripting environment. It was introduced to address some of the limitations of the mongo shell and provide a better interface for interacting with MongoDB databases.


## Centos install

A bit old, but still informational.

https://www.digitalocean.com/community/articles/how-to-install-and-run-a-node-js-app-on-centos-6-4-64bit

http://docs.mongodb.org/manual/tutorial/install-mongodb-on-red-hat-centos-or-fedora-linux/



## INSTALL
Installing dependancies could be done with package.json
npm install


or manually with
npm install express
npm install mongodb
npm install logger

Start with node master.js

The file index.html contain links to all important pages

adduser cc
passwd cc

# Build bing
   cd bing
   replace  key: 'Bing Key Here',
   npm run build
   cp dist/assets/index.678987ec.js ../public/assets/index.419ece67.js 
   cp dist/assets/index.b26c6729.css ../public/assets/index.b26c6729.css 

# DATABASE commands


Useful database commands,

   database cc

   Contains collections
   users
   chickens


   > mongosh or mongo
   > use cc
   > db.users.find()
   > show collections

   > db.users.drop()

# Import chickens
   mongorestore dump


# Updated version

   chickens.html is updated to use w2ui-2.0 and has working save and delete.
   users/user.html still uses older w2ui and interface is not updated completely.

# TODO, monitor with process manager

   FROM node:latest
   WORKDIR /app
   COPY package.json /app
   RUN npm install && npm install pm2 -g
   COPY . /app
   CMD ["pm2-runtime", "start", "yourApp.js"]

# https

   openssl req -nodes -new -x509 -keyout server.key -out server.cert
   This command creates a new SSL certificate (server.cert) and a private key (server.key).


# On droplet
   https://docs.digitalocean.com/support/how-do-i-install-an-ssl-certificate-on-a-droplet/

   One verification method is http serve of a file.
    

   https://www.freenom.com/en/index.html?lang=en 

