# The openvocs API

The openvocs API is the client interface for openvocs communication. Using the API allows to build dedicated clients. Implementing the API allows to build dedicated servers. The API is the minimum information exchange definition required to run openvocs services. 

## Client API

### Logout

Logout is used to close a user session. 

```
{
	"event" : "logout"
}
```

### Login

Login is used to start a user session. 

The Response to this event should contain a sessionid to be used for login updates. 

```
{
	"event" : "login",
	"client" : <client uuid>,
	"parameter" :
	{
		"user" : <username>,
		"password" : <user password or session id if session was established>
	}
}
```

### Update Login

Update Login is used to update a session and to prevent the session closure. 

A session is a temporary login stored within the browser to prevent storage of user passwords. Instead of some password some session id will be used. 
Once the client lost a connection to the server, the session can be restored automatically without a new login. 

```
{
	"event" : "update_login",
	"client" : <client uuid>,
	"parameter" :
	{
		"user" : <username>,
		"session" : <session id>
	}
}
```

### Media

Media is used to transfer media parameter between client and server. 

```
{
	"event" : "media",
	"client" : <client uuid>,
	"parameter" :
	{
		"sdp" : <session description>,
		"type" : <"request" | "offer" | "answer">
	}
}
```

### Candidate

Candidate is used to transfer ICE candidates between client and server.

```
{
	"event" : "candidate",
	"client" : <client uuid>,
	"parameter" :
	{
		"candidate": <candidate_string>,
		"SDPMlineIndex" : <sdp media description index>,
		"SDPMid" : <sdp media id>,
		"ufrag" : <user fragment>,
	}
}
```

### End of Candidates

End of Candidates is used to transfer ICE candidates end between client and server.

```
{
	"event" : "end_of_candidates",
	"client" : <client uuid>,
	"parameter" : {}
}
```

### Client authorize

Authorize a session. (Select role)

```
{
	"event" : "update_login",
	"client" : <client uuid>,
	"parameter" :
	{
		"role" : <rolename>
	}
}

### Client Get

Get some information from server. Get user must be implemented to get the users data. 

```
{
	"event" : "get",
	"client" : <client uuid>,
	"parameter" :
	{
		"type" : "user"
	}
}

### User Roles

Get user roles information from server.
Returns a list of usre roles. 

```
{
	"event" : "user_roles",
	"client" : <client uuid>,
	"parameter" :{}
}

### Role Loops

Get role loops information from server. 
Returns a list of user loops. 

```
{
	"event" : "role_loops",
	"client" : <client uuid>,
	"parameter" :{}
}

### Get Recodings

Get a list of recordings from server.
Returns a list of recordings. 

```
{
	"event" : "get_recordings",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <looname>
		"user" : <optional username>
		"from" : <from epoch>
		"to"   : <to epoch>
	}
}

### SIP Status

Get the SIP status from server. 
If a SIP gateway is connected the result will contain a SIP:true. 

```
{
	"event" : "role_loops",
	"client" : <client uuid>,
	"parameter" :{}
}

### Switch loop state

Switch a loopstate. 

```
{
	"event" : "switch_state",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <loopname>,
		"state": <"recv" | "send" | "none">
	}
}

### Switch loop volume

Switch loop volume. 

```
{
	"event" : "switch_volume",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <loopname>,
		"volume": <0 .. 100>
	}
}

### Switch Push To Talk

Switch Push to Talk state. 

```
{
	"event" : "switch_ptt",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <loopname>,
		"state": <true, false>
	}
}

### Call from a loop 

Call from a loop to some SIP number. 
Works only with sip gateway connected. 

```
{
	"event" : "call",
	"client" : <client uuid>,
	"parameter" :
	{
		"loop" : <loopname>,
		"dest" : <destination number>,
		"from" : <optional from number>
	}
}

### Hangup a call from a loop 

Hangup a call. 
Works only with sip gateway connected. 

```
{
	"event" : "hangup",
	"client" : <client uuid>,
	"parameter" :
	{
		"call" : <callid>,
		"loop" : <loopname>
	}
}

### Permit a call to a loop 

Permit some callin to some loop. 
Works only with sip gateway connected. 

```
{
	"event" : "permit",
	"client" : <client uuid>,
	"parameter" :
	{
		"caller" : <callerid>,
		"callee" : <calleeid>,
		"loop"   : <loopname>,
		"from"   : <from epoch>
		"to"     : <to epoch>
 	}
}

### Revoke a call to a loop 

Revoke a previously set permission of some callin to some loop. 
Works only with sip gateway connected. 

```
{
	"event" : "revoke",
	"client" : <client uuid>,
	"parameter" :
	{
		"caller" : <callerid>,
		"callee" : <calleeid>,
		"loop"   : <loopname>,
		"from"   : <from epoch>
		"to"     : <to epoch>
 	}
}

### List active calls 

List active calls. 
Works only with sip gateway connected. 

```
{
	"event" : "list_calls",
	"client" : <client uuid>,
	"parameter" :{}
}

### List active call permissions

List active call permissions. 
Works only with sip gateway connected. 

```
{
	"event" : "list_permissions",
	"client" : <client uuid>,
	"parameter" :{}
}

### List current SIP status

List SIP status. 
Works only with sip gateway connected. 

```
{
	"event" : "list_sip",
	"client" : <client uuid>,
	"parameter" :{}
}

### Set Keyset layout

Set a keyset layout of some client. 

```
{
	"event" : "set_keyset_layout",
	"client" : <client uuid>,
	"parameter" :
	{
		"domain" : "<domainname>",
        "name"   : "<name>",
        "layout"   : {}
	}
}

### Get Keyset layout

Get a keyset layout for some client. 

```
{
	"event" : "get_keyset_layout",
	"client" : <client uuid>,
	"parameter" :
	{
		"domain" : "<domainname>",
        "layout"   : "layoutname"
	}
}

### Set user data

Set the users data.

```
{
	"event" : "set_user_data",
	"client" : <client uuid>,
	"parameter" :
	{
		...
	}
}

### Get user data

Get the users data.

```
{
	"event" : "get_user_data",
	"client" : <client uuid>,
	"parameter" : {}
}

### Register a client

Register a client to receive systembroadcasts. 

```
{
	"event" : "register",
	"client" : <client uuid>,
	"parameter" : {}
}