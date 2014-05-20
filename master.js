/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


//w2utils.settings.date_format:

var express = require('express');
var bodyParser = require('body-parser');
var auth = require("basic-auth");

var app = require('express')()
  , server = require('http').createServer(app)
  , io = require('socket.io').listen(server);

var port = 10180;

var morgan = require('morgan');
var users = require('./users/server.js');
var chickens = require('./chickens/server.js');


var userList = [ "admin" , "Ellinor" , "Olle" , "Alex" , "Ludvig" , "Anna" , "Guest" ];

app.use(bodyParser());
app.use(morgan("dev"));

// Authenticator
// http://blog.modulus.io/nodejs-and-express-basic-authentication


var passport = require('passport');
var BasicStrategy = require('passport-http').BasicStrategy;

//var lastUser=Olle;
        
passport.use(new BasicStrategy(
  function(username, password, done) {
      //console.log("AUTH!!!");
      // username.valueOf() === 'olle' &&
      
    if (password.valueOf() === 'pip')
      return done(null, true);
    else
      return done(null, false);
  }
));

app.use(passport.initialize());
app.use(passport.session());
app.use('/',passport.authenticate('basic', { session: false }));
app.use('/',express.static(__dirname + '/public'));


function testMe(req,res) {
  res.sendfile('public/index.html');
};


//app.get('/users/users.html', function(req,res) {
//  res.sendfile('users/users.html');
//});
//app.use(express.static(__dirname + '/users/users.html'));


var fs  = require('fs')




app.get('/js/application2.js',passport.authenticate('basic', { session: false }),function(req,res) {
  var user = auth(req);
  var response; //  = 'public/js/application.js';
  fs.readFile('public/js/application.js','utf-8', function(e,d) {
      if (e) {
        console.log(e);
        res.send(500, 'Something went wrong');
      }
      else {
        console.log("serving custom file",user.name,user.pass);
        //fs.writeFile('counter.txt',parseInt(d) + 1);
        response = "var username= \"" + user.name + "\";";
        response += d;
        res.send(response);
      }
  })  
  //res.sendfile('public/js/application.js');
});


app.get('/' , passport.authenticate('basic', { session: false }),testMe);



app.get("/about", function(request, response) {
  response.writeHead(200, { "Content-Type": "text/plain" });
  response.end("Welcome to the about page!");
});



// delete to see more logs from sockets
io.set('log level', 1);

io.sockets.on('connection', function (client) {
    
        console.log("connected");
        chickens.serveChickens(client);
    
	client.on('send:coords', function (data) {
                //console.log(data);
                users.savePosition(data);
                
                users.servePosition(client);
  	});
});

// Not necessary??
app.set('json spaces', 0);


//app.get("/users/users.json", users.getUsers);
app.post("/users/records.json", users.usersPost);

app.post("/chickens/records.json", chickens.chickensPost);


//app.listen(port);
server.listen(port);
console.log('Your server goes on localhost:' + port);