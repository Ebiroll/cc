Chicken chase
==============


INSTALL node & mongodb 
https://www.digitalocean.com/community/articles/how-to-install-and-run-a-node-js-app-on-centos-6-4-64bit

http://docs.mongodb.org/manual/tutorial/install-mongodb-on-red-hat-centos-or-fedora-linux/


adduser cc
passwd cc

git clone ssh://github.com/Ebiroll/cc.git


INSTALL
=========
Installing dependancies could be done with package.json
npm install


or manually with
npm install express
npm install mongodb
npm install body-parser
npm install logger

Start with node master.js

The file index.html contain links to all important pages

DATABASE
=========

Useful database commands,

database mms
Contains collections
users


>mongo
>use cc
>db.users.find()
>show collections

>db.users.drop()