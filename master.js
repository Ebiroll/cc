/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


//w2utils.settings.date_format:

var express = require('express');
var bodyParser = require('body-parser');
//var logger = require('logger');
var app = express();

// 
var users = require('./users/server.js');


var userList = [ "admin" , "Ellinor" , "Olle" , "Alex" , "Ludvig" , "Anna" , "Guest" ];

app.use(bodyParser());
//app.use(express.logger("dev"));
app.use(express.static(__dirname + '/public'));

app.get('/', function(req,res) {
  res.sendfile('public/index.html');
});

//app.get('/users/users.html', function(req,res) {
//  res.sendfile('users/users.html');
//});
//app.use(express.static(__dirname + '/users/users.html'));


var myAuthCallback=function(error,result)
{
   console.log("MyAuthCallback");
   return(result);
}

// Authenticator
// http://blog.modulus.io/nodejs-and-express-basic-authentication
app.use(express.basicAuth(function(user, pass) {
    var ok=true;
    
   var result = (user === 'admin' && pass === 'tupp');
   // Only admin allowed before we provide callback funcrion
    // user === 'test'
    //pass === 'test';
   console.log("Authenticate");
   myAuthCallback(null /* error */, result);
   return(result)
 }));


app.get("/about", function(request, response) {
  response.writeHead(200, { "Content-Type": "text/plain" });
  response.end("Welcome to the about page!");
});

// Not necessary??
app.set('json spaces', 0);


//app.get("/users/users.json", users.getUsers);
app.post("/users/records.json", users.usersPost);


app.listen(10080);
