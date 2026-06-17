// Ideas

// Server : { Connection, Connection, Connection, ... }
// Connection: Request, Response, Socket, Resource?

// Server accepts a new Connection
// The Connection's Request is built
// A Resource is created depending on the Request
// A Response is created using the Request and the Resource
// Connection is kept alive or closed depending on configuration/Request
// Flow: Server -> Connection -> Request -> Resource -> Response -> Connection -> Server

// An active server listening for connections
class Server {
	// Connections
	// Listening Socket
	// Config informations
};

// A connection between a server and a client
class Connection {
	// Request
	// Response
	// Client Socket
	// State
};

class Request {
	// Headers
	// Build itself from received client request
	// Parse itself after receiving complete request data
};

class Response {
	// Headers
	// Status code + description
	// Build itself from Request and Resource
};

// Socket abstraction
class Socket {
};

// Request/Response headers
class Headers {
	// Key->Value pairs
};

// Interface for a resource requested by the user
class Resource {
public:
	virtual ~Resource() = 0;
};

// Static resource, i.e. normal file
class StaticResource: public Resource {
};

// CGI script to generate a dynamic resource
class CGIResource: public Resource { // or DynamicResource
};

// Directory
class DirectoryResource: public Resource {
};
