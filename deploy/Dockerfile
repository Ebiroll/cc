# Use the official MongoDB image as the base image
FROM mongo:latest

# (Optional) Copy custom scripts or configurations if you have any
# For example, to copy a custom script:
# COPY ./my-mongo-setup.sh /docker-entrypoint-initdb.d/

# Set environment variables for MongoDB initialization (uncomment if needed)
# These are the same variables you commented out in your docker-compose snippet
# ENV MONGO_INITDB_ROOT_USERNAME=root
# ENV MONGO_INITDB_ROOT_PASSWORD=example
# ENV MONGO_INITDB_DATABASE=your-database-here

# The standard MongoDB port
EXPOSE 27017

# The entrypoint is inherited from the base image, so MongoDB will start as usual
# No CMD needed because the base image already defines how MongoDB should start