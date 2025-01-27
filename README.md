# Associative Storage Engine
This repository contains all files for the Associative Storage Engine project.

## The Project
The purpose of this Project was to create an in-memory database similiar to redis, that also supports DMA.

## Structure
- `engine/`
- `web/`
  - `java-driver/`
  - `backend/`
  - `admin-panel/`

*engine* contains the CPP code of the project which includes the core DB, DMA and a TCP-Server.

*web/java-driver* contains a TCP-Client implemented in Java.

*web/backend* contains a HTTP backend implemented in Java that utilizes the java-driver.

*web/admin-panel* contains a React web application that connects to the backend.

Each of this application contains it's own README for further explainations.
