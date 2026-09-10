_This project has been created as part of the 42 curriculum by kdonlon, nhoussie, mamarti._

---

### Table of Contents

- [Description](#description)
- [Instructions](#instructions)
  - [Building](#building)
  - [Running the Server](#running-the-server)
  - [Configuration File](#configuration-file)
    - [Blocks](#blocks)
    - [Directives](#directives)
    - [Configuration Inheritance](#configuration-inheritance)
- [Resources](#resources)

---

## Description

This project is an HTTP server written in C++98.

The goal of the project is to understand how HTTP servers work internally.

The server can be accessed using standard web browsers and supports the `GET`, `POST` and `DELETE` HTTP methods, file uploads, CGI execution, directory listings and HTTP redirections.

The server is configured by providing a configuration file as a command-line argument.

## Instructions

### Building

```sh
make
```

### Running the Server

```sh
./webserv <configuration_file>
```

Example configuration files are provided in the repository.

Once the server is running, it can be accessed from a web browser or tested using command-line tools such as `curl`.

### Configuration File

The server requires a configuration file as a command-line argument.

The configuration file uses "blocks" and "directives".

#### Blocks

Blocks use the following format:

```
blockType {
    ...
}
```

Two block types are supported:

- `server`: defines the configuration of a server
- `route`: defines the configuration of a route

A `route` block must be declared inside a `server` block.

#### Directives

A directive consists of a key-value pair separated by a colon:

```
key <arg>: value
```

| Directive                    | Description                                                                                             |
| ---------------------------- | ------------------------------------------------------------------------------------------------------- |
| `autoindex`                  | Defines if directory listing is allowed                                                                 |
| `cgi <extension>`            | The CGI executable used to handle files with the specified extension                                    |
| `error_page <code\|default>` | The page returned for a given HTTP error status code. `default` specifies the fallback error page       |
| `index`                      | The default file(s) to serve if a requested resource is a directory                                     |
| `listen`                     | The interface and port on which the server listens. The format can be `interface:port` or simply `port` |
| `max_body_size`              | The maximum size allowed for a client request body                                                      |
| `methods`                    | HTTP methods allowed for a route. Methods are separated by commas                                       |
| `redirect`                   | A path to which the user should be redirected to access the resource                                    |
| `root`                       | The path to the directory corresponding to the given route                                              |
| `upload`                     | Defines if uploading files is allowed                                                                   |
| `upload_dir`                 | The directory to which uploaded files are stored                                                        |

#### Configuration Inheritance

Directives defined in a parent block are inherited by child blocks when allowed and when they are not redefined.

```
server {
    root: ./
    index: index.html
    methods: GET

    route /data {
        root: ./data
    }
}
```

In this example, the `/data` route inherits `index` and `methods` from the `server` block and redefines `root`.

Configuration files must follow the syntax and rules expected by the server. Invalid configuration files will result in a configuration error.

## Resources

- [MDN Web Docs - HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP) - Reference for HTTP concepts and related web technologies.
- [MDN Web Docs - HTTP Messages](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Messages) - Useful resource to understand HTTP requests and responses.
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - An introduction to network programming, client-server communication and sockets.

AI tools were used during the project, mainly for designing/reviewing test cases and for code review.
