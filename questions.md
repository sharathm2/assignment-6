1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

The remote client determines when a command's output is fully received by looking for the end of message character. This is done by checking for the EOF character being sent from the server, which is denoted by RDSH_EOF_CHAR. After the command output is recieved, the server sends an EOF character to signal the end of the message. The client then knows that the command output has been fully received. In order to handle partial reads and to ensure complete message transmission, we can implement a loop within our client that continues to read from the server until the EOF character is recieved.

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

A networked shell protocol should define and detect the beginning and end of a command sent over a TCP connection by using delimiters or markers. In our implementation, we use the EOF character to denote the end of a command. The beginning of a command is detected by the client when the server sends the first message. If message boundaries are not handled correctly, the client could misinterpret the data recieved, which could lead to incomplete or incorrect command execution. 

3. Describe the general differences between stateful and stateless protocols.

Stateful protocols are protocols that maintain the state of a session across many requests. This means that the server keeps track of the clients state, allowing for more complex interactions that depend on previous requests. A few examples of this would be HTTP and FTP with sessions(much like how websites use cookies to keep track of a user's state). Stateless protocols are protocols that treat each request as an independent request, and does not maintain or keep track of any previous requests or states. The server will not retain any information regarding the client's state. An example of this would be TCP.

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

Although UDP may be considered "unreliable" due to the lack of a guaranteed message delivery and lack of error checking, it is still useful in certain situations due to how lightweight it is, and how low latency it can provide. Examples such as video games and video streaming use UDP due to the flexibility in packet loss, and the low latency it can provide.

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

The operating system provides a socket interface to enable applications to use network communications. Sockets are the endpoints of a connection and allow for applications to send and recieve data over a network using TCP or UDP protocols. The socket API makes it easier for developers to use network communications by abstracting the complexity of network protocols.