# Overview of ov_vocs_db

ov_vocs_db is some inmemory db for a structured scheme of JSON items. The sctructure is realated to the management of vocs specific items like domains, projects, roles, loops and user entries. It implementes some ROLE BASED ACCESS CONTROL (RBAC) environment integrated with some domain and project structure management.

A domain may contain a set of user, role and loop entries, as well as a set of
projects, which each may contain some own set of user, role and loop entries.
An example scheme structure is defined as follows:

```json
{
	"domain_id" :
	{
		"id": "domain_id",
		"users" : {},
		"roles" : {},
		"loops" : {},
		"projects":
		{
			"project_id" :
			{
				"id" : "project_id",
				"users" : {},
				"roles" : {},
				"loops" : {}
			}
		}
	}
}
```
The scheme uses predefined keywords, which are **users**, **roles**, **loops**, **projects** as well as **id**.


Within ov_vocs_db 5 different entity types are managed, each entity type must contain unique IDs. Different entity types may use same ID e.g. a role may use the ID flightdirector@domain1 and some loop may use the same ID.

1. user
2. role
3. loop
4. project
5. domain

## Components of the database

The database contains of 3 main components:

1. ov_vocs_db - the inmemory database
2. ov_vocs_db_persistance - the persistance layer of the database
3. ov_vocs_db_service - the service layer of the database for external requests

Each of the 3 components implements a different set of functions required for overall functionalitiy of the RBAC DB.

### ov_vocs_db

Main component of the DB is ov_vocs_db, which contains the inmemory storage of data. All functionalities required to populate and request VOCS related RBAC information are implemented here. In addition to the RBAC related dataset actual selected states may be stored for loops and will be reapplied at login.
This component is implemented to be used directly by some VOCS implementation and provide all access control and manage all access selected states for some VOCS system. This component is the core of some VOCS in terms of access control and access selection.

### ov_vocs_db_persistance

As persistance component a SNAPSHOT based storage system is used. All data of ov_vocs_db may be persistant within the filesystem and reloaded from the filesystem.
Snapshots are configurable with timeframe of a 1 sec resolution. Maximum performance of the persistance functionality is storing the DB every second. As changes within the authentication layer and state layer may vary both timespans are configurable independently. It is expected to use the state_snapshot functionaltiy, but to persist authentication data on request during change of the datasets.

### ov_vocs_db_service

This component implements the remote API used for RBAC control and management.
It initeracts with the inmemory database as well as the persitance layer and provides functionaly to store and reload the dataset from disk.

Within the remote API functions to get, create, delete and update data is implemented. Data is clustered in ov_vocs_db_entity types, which are:

1. domain
2. project
3. loop
4. role
5. user

All of these objects may be managed on a CRUD (create/read/update/delete) basis.
In addition subitems of the entities may be managed CRUD based, with access over its key.

#### Implementation overview

1. any (sub) object must be created before content may be set
2. any update will add data, but not delete some (sub) keys of data
3. to delete some (sub) key use the provided delete functionality
4. administration is level in project access and domain access

#### Service API

The service API is provided via some websocket based JSON API.

#### Login

Login to the system as some user, project admin or domain admin.

##### request
```json
{
	"event":"login",
	"parameter":
	{
		"password":"user11",
		"user":"user11"
	},
	"uuid":"1-2-3-4"
}
```

##### response
```json
{
	"event":"login",
	"request":
	{
		"event":"login",
		"parameter":
		{
			"password":"user11",
			"user":"user11"
		},
		"uuid":"1-2-3-4"
	},
	"response":
	{
		"id":"user11"
	},
	"type":"unicast",
	"uuid":"1-2-3-4"
}
```

#### Logout

Logout from the system.

##### request
```json
{
	"event":"logout",
	"uuid":"1-2-3-4"
}
```

#### CREATE

Create some new object. Each object MUST be created in some scope, which is either a domain or some project. For example a user object may be set as domain user or project user. Scope of a project MUST be domain. Projects are managed under domains only.

**NOTE** Create a new domain is currently not supported and MUST be done externaly within the filesystem before loading the data.

##### request
```json
{
	"event" : "create",
	"parameter" :
	{
	    "type"  : "<domain | project | user | role | loop>",
	    "id"    : "<id to create>",
	    "scope" :
	    {
	        "type": "<domain | project>",
	        "id" : "<parent scope id>"
	    }
	}
}
```

##### response
```json
{
	"event":"create",
	"request":
	{
		"event":"create",
		"parameter":
		{
			"id":"user14",
			"scope":
			{
				"id":"project1",
				"type":"project"
			},
			"type":"user"
		},
		"uuid":"1-2-3-4"
	},
	"response":{},
	"type":"unicast",
	"uuid":"1-2-3-4"
}
```

#### GET

Get some entire entity object from the database. The result will contain the whole entity object including all subobjects.

##### request
```json
{
	"event" : "get",
	"parameter" :
	{
		"type" : "<domain | project | user | role | loop>",
		"id" : "<id to get>",
	},
	"uuid":"1-2-3-4"
}
```

##### response
```json
{
	"event":"get",
	"request":
	{
		"event":"get",
		"parameter":
		{
			"id":"loop11",
			"type":"loop"
		},
		"uuid":"1-2-3-4"
	},
	"response":
	{
		"result":
		{
			"id":"loop11",
			"roles":
			{
				"role11":true,
				"role12":true,
				"role13":true,
				"role21":false,
				"role31":false
			}
		},
		"type":"loop"
	},
	"type":"unicast",
	"uuid":"1-2-3-4"
}
```

#### UPDATE

Update some entity with the content provided. Will add new keys to the stored dataset, but not delete previously set keys.

##### request
```json
{
	"event" : "update",
	"parameter" :
	{
	    "type"  : "<domain | project | user | role | loop>",
	    "id"    : "<id to update>",
	    "data"  : { "<data to update>" }
	}
}
```

##### response
```json
{
	"event":"update",
	"request":
	{
		"event":"update",
		"parameter":
		{
			"data":
			{
				"name":"loop11"
			},
			"id":"loop11",
			"type":"loop"
		},
		"uuid":"1-2-3-4"
	},
	"response":{},
	"type":"unicast",
	"uuid":"1-2-3-4"
}
```

#### DELETE

Delete some entity from the DB.

##### request
```json
{
    "event" : "delete",
    "parameter" :
    {
        "type"  : "<domain | project | user | role | loop>",
        "id"    : "<id to delete>"
    }
}
```

##### response
```json
{
	"event":"delete",
	"request":
	{
		"event":"delete",
		"parameter":
		{
			"id":"user13",
			"type":"user"
		},
		"uuid":"1-2-3-4"
	},
	"response":{},
	"type":"unicast",
	"uuid":"1-2-3-4"
}
```

#### GET KEY

Get some key of some entity e.g. the users of some project.

##### request
```json
{
    "event" : "get_key",
    "parameter" :
    {
        "type"     : "<domain | project | user | role | loop>",
        "id" : "<id to get>",
        "key" : "<key to get>"
    }
}
```
##### response
```json
{
	"event":"get_key",
	"request":
	{
		"event":"get_key",
		"parameter":
		{
			"id":"loop11",
			"key":"id",
			"type":"loop"
		},
		"uuid":"1-2-3-4"
	},
	"response":
	{
		"id":"loop11",
		"key":"id",
		"result":"loop11",
		"type":"loop"
	},
	"type":"unicast",
	"uuid":"1-2-3-4"
}
```

#### UPDATE KEY

Update some key of some entity with some data. E.g. update the users object of some role.

##### request
```json
{
    "event" : "update_key",
    "parameter" :
    {
        "type"  : "<domain | project | user>",
        "id"    : "<id to update>",
        "key"   : "<key to update>",
        "data"  : { "<data to update>" }
    }
}
```

##### response
```json
{
	"event":"update_key",
	"request":
	{
		"event":"update_key",
		"parameter":
		{
			"data":null,
			"id":"loop11",
			"key":"x",
			"type":"loop"
		},
		"uuid":"1-2-3-4"
	},
	"response":{},
	"type":"unicast",
	"uuid":"1-2-3-4"
}
```

#### DELETE KEY

Delete some key of some entity.

##### request
```json
{
    "event" : "delete_key",
    "parameter" :
    {
        "type"  : "<domain | project | user>",
        "id"    : "<id to update>",
        "key"   : "<key to update>"
    }
}
```

##### response
```json
{
	"event":"delete_key",
	"request":
	{
		"event":"delete_key",
		"parameter":
		{
			"id":"loop12",
			"key":"roles",
			"type":"loop"
		},
		"uuid":"1-2-3-4"
	},
	"response":{},
	"type":"unicast",
	"uuid":"1-2-3-4"
}
```


#### ADMIN DOMAINS

Get a list of the domains the currently logged in user is admin of.

##### request
```json
{
    "event" : "admin_domains",
}
```

##### response
```json
"domains": 
[
	"localhost"
]
```

#### ADMIN PROJECTS

Get a list of the projects the currently logged in user is admin of.

##### request
```json
{
    "event" : "admin_projects",
}
```
##### response
```json
"domains": 
{
	"event": "admin_projects",
	"request": {
	"event": "admin_projects",
    "uuid": "d49f6f4a-ce6f-6aee-83b6-547f55b758b1"
  },
  "response": {
    "projects": {
      "project@localhost": {
        "domain": "project@localhost",
        "id": "project@localhost",
        "name": null
      }
    }
  },
  "type": "unicast",
  "uuid": "d49f6f4a-ce6f-6aee-83b6-547f55b758b1"
}
```

#### SAVE

Trigger persistance save functionaltiy.
**NOTE** Logged in user MUST be some domain admin.

##### request
```json
{
    "event" : "save",
}
```

#### LOAD

Trigger persistance load functionaltiy.
**NOTE** Logged in user MUST be some domain admin.

##### request
```json
{
    "event" : "load",
}
```












