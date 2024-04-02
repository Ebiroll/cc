
Manual deployment of the docker-compose file in podman

Podman, while similar to Docker in many ways, handles networking a bit differently and does not directly support the network_mode: service:<name> syntax from Docker Compose.
However, you can achieve a similar effect by using Podman's pod feature, which groups containers into a shared network namespace (among other shared namespaces)
where they can communicate with each other as if they were on the same localhost.

If you want to run a simple dev and test mongo server,

############# Dev container for mongodb

If you want to develop locally and connect to a podman mongodb container:

podman pull docker.io/mongodb/mongodb-community-server:latest

podman images 

podman volume create mongodb-data

podman run -d --name devDB -p 27017:27017 -v /path/to/host/data:/data/db docker.io/mongodb/mongodb-community-server:latest

#############

# Dev setup with a pod and mongodb:latest

podman pod create --name mypod -p 10180:10180

cd deploy

podman build   --tag mongodb:latest .

cd ../.devcontainer

podman build   --tag node-mongotools:latest .

podman volume create mongodb-data


// Instead of this,
// podman run -d --name mongodb -v mongodb-data:/data/db mongodb:latest sleep infinity

# Start container in pod
podman run -d --pod mypod --name db -v mongodb-data:/data/db mongodb:latest  sleep infinity

podman run --pod mypod --name app -p  node-mongotools:latest sleep infinity



# Another options is to start containers in two different windows

podman run --pod mypod --name db -v mongodb-data:/data/db mongodb:latest 

podman run --pod mypod --name app -p 10180:10180 -v .:/workspace node-mongotools:latest /bin/bash

# I could not get localhost from windows to access the container.
# This was a workaround that worked. wsl 
 sudo socat TCP-LISTEN:80,fork TCP:localhost:10180

