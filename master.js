import express, { urlencoded, json } from 'express';
var app = express();
app.use(urlencoded({extended: true}));
app.use(json()); // To parse the incoming requests with JSON payloads
import auth from "basic-auth";
import { fileURLToPath } from 'url';

import morgan from 'morgan';
import { usersAdd, savePosition, servePosition, usersPost, usersGet, positionPost } from './users/server.js';
import {saveAIChickens, serveChickens, chickensPost, chickensGet } from './chickens/server.js';

import OpenAI from 'openai';


const openai = new OpenAI({
  apiKey: 'nvapi-RRGetFromNVidia',
  baseURL: 'https://integrate.api.nvidia.com/v1',
})

// Similar to main_test but return result of chat message as a short string max 100 characters
async function chat_ai(message) {
  const completion = await openai.chat.completions.create({
    model: "mistralai/mixtral-8x22b-instruct-v0.1",
    messages: [{
      "role": "user",
      "content": "Reply with maximum 100 characters," + message
    }],
    temperature: 0.5,
    top_p: 1,
    max_tokens: 1024,
    stream: true,
  });
  
  let fullResponse = '';

  for await (const chunk of completion) {
    if (chunk.choices[0]?.delta?.content) {
      fullResponse += chunk.choices[0].delta.content;
    }
  }

  console.log("Full Response:", fullResponse);

  return fullResponse;
}


async function main_test(lat_lng) {
  const completion = await openai.chat.completions.create({
    model: "mistralai/mixtral-8x22b-instruct-v0.1",
    messages: [{
      "role": "user",
      "content": "Can you generate the latlong coordinates as JSON for 5 interesting points near the point" + lat_lng + ". Use 'name' for the name of the location and 'description' for the long description, 'lat' and 'lng' witout nesting. Return no other text. It should be lesser known locations."
    }],
    temperature: 0.5,
    top_p: 1,
    max_tokens: 1024,
    stream: true,
  });
  
  let fullResponse = '';

  for await (const chunk of completion) {
    if (chunk.choices[0]?.delta?.content) {
      fullResponse += chunk.choices[0].delta.content;
    }
  }

  console.log("Full Response:", fullResponse);

  try {
    const array = JSON.parse(fullResponse);
    saveAIChickens(array[0]);
    saveAIChickens(array[1]);
    saveAIChickens(array[2]);
    saveAIChickens(array[3]);
    saveAIChickens(array[4]);

  } catch (error) {
    console.error("Error parsing JSON:", error);
  }
}

// Call the function to test
// main_test("59.3269708858495, 18.071861248378614");

var port = 10180;

//var port = 443;
 
import { createServer } from 'http';
import { createServer as _createServer } from 'https';
import { readFileSync, readFile } from 'fs';
import path from 'path';
import socketio from 'socket.io';
//const easyrtc = require("open-easyrtc");      // EasyRTC external module
import { resolve, dirname } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

var server;
var io;

// Check if running in production
const isProduction = process.env.NODE_ENV === 'production';

if (isProduction) { // 
    // Production environment - Use HTTPS
    const certOptions = {
        key: readFileSync(resolve('server.key')),
        cert: readFileSync(resolve('server.cert')),
        // Include CA chain if necessary
    };
    
    server = _createServer(certOptions, app);
} else {
    // Development or other non-production environment - Use HTTP
    server = createServer(app);
}

io = socketio(server);
// const io = require("socket.io")(webServer);


server.listen(process.env.PORT || port, () => {
    console.log(`Server is running on port ${server.address().port}`);
});


//var server = require('http').createServer(app);
//var io = require('socket.io')(server);
//server.listen(process.env.PORT || port);
//console.log('Server is running ');




app.use(morgan("dev"));

// Authenticator
// http://blog.modulus.io/nodejs-and-express-basic-authentication


//import { use, initialize, session as _session, authenticate } from 'passport';
import { BasicStrategy } from 'passport-http';
import passport from 'passport';

const { use, initialize, session, authenticate } = passport; // Destructure directly


// Serve the bundle in-memory in development (needs to be before the express.static)
//if (process.env.NODE_ENV === "development") {
//  const webpackMiddleware = require("webpack-dev-middleware");
//  const webpack = require("webpack");
//  const config = require("webpack.config");
//
//  app.use(
//    webpackMiddleware(webpack(config), {
//      publicPath: "/dist/"
//    })
//  );
//}


//app.use(static('js'));
app.use(express.static(path.join(__dirname, 'js')));
        
passport.use(new BasicStrategy(
  (username, password, done) => {
    // Basic authentication logic
    if (password === 'pip') {
      usersAdd(username);   
      return done(null, { username: username });
    } else {
      return done(null, false);
    }
  }
));

// Initialize Passport
app.use(passport.initialize());


//app.use(require('express-session')({ 
//  secret: 'Enter your secret key',
//  resave: true,
//  saveUninitialized: true
//}));

//app.use(initialize());
//app.use(session());

//app.use('/assets',static(__dirname + '/public/assets'));
app.use('/assets', express.static(path.join(__dirname, 'public/assets')));

// Authenticated route for serving custom JavaScript
app.get('/js/application2.js', 
  passport.authenticate('basic', { session: false }),
  (req, res) => {
    const user = req.user; // The username passed from the authentication

    console.log("USERNAME===============", user);

    if (!user) {
      return res.sendStatus(401); // Unauthorized if authentication fails
  }

  // 2. Extract Username
  const username = user.username; 

    readFile(path.join(__dirname, 'public/js/application2.js'), 'utf8', (error, data) => {
      if (error) {
        console.error(error);
        return res.sendStatus(500); // Send 500 Internal Server Error
      }
      // Dynamically prepend the username variable to the script
      const customScript = `var username = "${username}";\n${data}`;
      res.type('application/javascript').send(customScript);
    });
  }
);

app.use('/js', express.static(path.join(__dirname, 'public/js')));
app.use('/home', express.static(path.join(__dirname, 'public/home')));
app.use('/lib', express.static(path.join(__dirname, 'public/lib')));
app.use('/img', express.static(path.join(__dirname, 'public/img')));
app.use('/css', express.static(path.join(__dirname, 'public/css')));
app.use('/fonts', express.static(path.join(__dirname, 'public/fonts')));
app.use('/index_files', express.static(path.join(__dirname, 'public/index_files')));

app.get('/chickens.html', function(req,res) {
  res.sendFile('chickens.html',{ root: 'public' });
});


function testMe(req,res) {

  res.sendFile('index.html',{ root: 'public' });
};

app.get('/result.html', function(req,res) {
  res.sendFile('result.html',{ root: 'public' });
});

app.get('/login.html', function(req,res) {
  res.sendFile('login.html',{ root: 'public' });
});

app.get('/users.html', function(req,res) {
  res.sendFile('users.html',{ root: 'public' });
});



app.get('/lmap.html', function(req,res) {
  res.sendFile('lmap.html',{ root: 'public' });
});

app.get('/gmap.html', function(req,res) {
    res.sendFile('gmap.html',{ root: 'public' });
});
  

app.get('/users/users.html', function(req,res) {
  res.sendFile('users/users.html',{ root: 'public' });
});

// app.use(express.static(__dirname + '/users/users.html'));


// Authenticated route for serving custom JavaScript with username
app.get('/js/user.js',
  passport.authenticate('basic', { session: false }),
  (req, res) => {
    const username = req.user; // The username passed from the authentication

    console.log("USERNAME===============", username);

    // Dynamically create a JavaScript snippet containing the username
    const response = `var username= "${username}";`;
    res.type('application/javascript').send(response);
  }
);


app.get('/index.html' , passport.authenticate('basic', { session: false }),testMe);

app.get('/' , passport.authenticate('basic', { session: false }),testMe);


// app.use('/',passport.authenticate('basic', { session: false }),testMe);


app.get("/about", function(request, response) {
  response.writeHead(200, { "Content-Type": "text/plain" });
  response.end("Welcome to the about page!");
});

// delete to see more logs from sockets
// io.set('log level', 2);
// io.set('log level', 3);

const maxOccupantsInRoom = 10;

const rooms = new Map();


io.on('connection', function (client) {
  console.log("user connected", client.id);

  serveChickens(client);

  let curRoom = null;

  client.on("joinRoom", data => {
    const { room } = data;
    curRoom = room;
    let roomInfo = rooms.get(room);
    if (!roomInfo) {
      roomInfo = {
        name: room,
        occupants: {},
        occupantsCount: 0
      };
      rooms.set(room, roomInfo);
    }

    if (roomInfo.occupantsCount >= maxOccupantsInRoom) {
      let availableRoomFound = false;
      const roomPrefix = `${room}--`;
      let numberOfInstances = 1;
      for (const [roomName, roomData] of rooms.entries()) {
        if (roomName.startsWith(roomPrefix)) {
          numberOfInstances++;
          if (roomData.occupantsCount < maxOccupantsInRoom) {
            availableRoomFound = true;
            curRoom = roomName;
            roomInfo = roomData;
            break;
          }
        }
      }

      if (!availableRoomFound) {
        const newRoomNumber = numberOfInstances + 1;
        curRoom = `${roomPrefix}${newRoomNumber}`;
        roomInfo = {
          name: curRoom,
          occupants: {},
          occupantsCount: 0
        };
        rooms.set(curRoom, roomInfo);
      }
    }

    const joinedTime = Date.now();
    roomInfo.occupants[client.id] = joinedTime;
    roomInfo.occupantsCount++;

    console.log(`${client.id} joined room ${curRoom}`);
    client.join(curRoom);

    client.emit("connectSuccess", { joinedTime });
    const occupants = roomInfo.occupants;
    io.in(curRoom).emit("occupantsChanged", { occupants });
  });

  client.on("send", data => {
    io.to(data.to).emit("send", data);
  });

  client.on("broadcast", data => {
    client.to(curRoom).emit("broadcast", data);
  });

  client.on("disconnect", () => {
    console.log('disconnected: ', client.id, curRoom);
    const roomInfo = rooms.get(curRoom);
    if (roomInfo) {
      console.log("user disconnected", client.id);

      delete roomInfo.occupants[client.id];
      roomInfo.occupantsCount--;
      const occupants = roomInfo.occupants;
      client.to(curRoom).emit("occupantsChanged", { occupants });

      if (roomInfo.occupantsCount === 0) {
        console.log("everybody left room");
        rooms.delete(curRoom);
      }
    }
  });

  client.on('chat message', async (msg) => { 
    console.log("chat", msg);
    // If message is add, call main_test() with the rest as a coordinate
    if (msg === "add") {
      //rest_of_message = msg.split(" ")[1];
      main_test("59.3269708858495, 18.071861248378614");
    }
    if (msg.startsWith("hello")) {
      // If message contains space
      if (msg.indexOf(" ") === -1) {
        msg= await chat_ai("Hi, how are you?");
        console.log("chat", msg);
      } else {
        const rest_of_message = msg.split(" ").slice(1).join(" ");
        console.log("ask ", rest_of_message);
        msg=await chat_ai(rest_of_message);
      }
    }


    io.emit('chat message', msg);
  });

  let lastCalled = {};  // Object to track the last time the function was called for each client

  client.on('send:coords', function (data) {
    const currentTime = Date.now();  // Current timestamp
    const throttlePeriod = 15000;  // Throttle period in milliseconds

    if (!lastCalled[client.id] || currentTime - lastCalled[client.id] > throttlePeriod) {
      lastCalled[client.id] = currentTime;  // Update the last called time
      savePosition(data, client);
      servePosition(client);
    } else {
      console.log(`Throttled 'send:coords' for client: ${client.id}`);
    }
  });
});


// Not necessary??
app.set('json spaces', 0);

app.post("/users/records.json", (req, res)=>{
  usersPost(req, res);
});

app.get("/users/records.json", (req, res)=>{
  usersGet(req, res);
});



app.post("/users/positions.json",  (req, res)=>{
  //var postData = req.body;
  positionPost(req, res);
});


app.post("/chickens/records.json", (req, res)=>{
  chickensPost(req, res);
});

app.get("/chickens/records.json", (req, res)=>{
  chickensGet(req, res);
});

//app.use('/chickens',express.static(__dirname + '/public/chickens'));

//app.listen(port);
//server.listen(port);





const myIceServers = [
  {"urls":"stun:stun1.l.google.com:19302"},
  {"urls":"stun:stun2.l.google.com:19302"},
  // {
  //   "urls":"turn:[ADDRESS]:[PORT]",
  //   "username":"[USERNAME]",
  //   "credential":"[CREDENTIAL]"
  // },
  // {
  //   "urls":"turn:[ADDRESS]:[PORT][?transport=tcp]",
  //   "username":"[USERNAME]",
  //   "credential":"[CREDENTIAL]"
  // }
];

//easyrtc.setOption("appIceServers", myIceServers);
//easyrtc.setOption("logLevel", "debug");
//easyrtc.setOption("demosEnable", false);
//easyrtc.setOption("logColorEnable", true);
//easyrtc.setOption("logObjectDepth", 5);

// Overriding the default easyrtcAuth listener, only so we can directly access its callback
//easyrtc.events.on("easyrtcAuth", (socket, easyrtcid, msg, socketCallback, callback) => {
//    easyrtc.events.defaultListeners.easyrtcAuth(socket, easyrtcid, msg, socketCallback, (err, connectionObj) => {
//        if (err || !msg.msgData || !msg.msgData.credential || !connectionObj) {
//            callback(err, connectionObj);
//           return;
//        }

//        connectionObj.setField("credential", msg.msgData.credential, {"isShared":false});

//        console.log("["+easyrtcid+"] Credential saved!", connectionObj.getFieldValueSync("credential"));

//        callback(err, connectionObj);
//   });
//});

// To test, lets print the credential to the console for every room join!
//easyrtc.events.on("roomJoin", (connectionObj, roomName, roomParameter, callback) => {
//    console.log("["+connectionObj.getEasyrtcid()+"] Credential retrieved!", connectionObj.getFieldValueSync("credential"));
//    easyrtc.events.defaultListeners.roomJoin(connectionObj, roomName, roomParameter, callback);
//});

// Start EasyRTC server
//easyrtc.listen(app, io, null, (err, rtcRef) => {
//    console.log("Initiated");

//    rtcRef.events.on("roomCreate", (appObj, creatorConnectionObj, roomName, roomOptions, callback) => {
//        console.log("roomCreate fired! Trying to create: " + roomName);

//        appObj.events.defaultListeners.roomCreate(appObj, creatorConnectionObj, roomName, roomOptions, callback);
//    });
//});


console.log('Your server goes on localhost:' + port);
