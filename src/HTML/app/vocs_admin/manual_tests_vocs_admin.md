# Testing routines for vocs admin interface (openvocs core)

These are the testing routines for the openvocs core admin interface only.

## Part 1: Operation & Backup server

### Prerequisites
* [ ] 2 openvocs servers (main & backup)
* [ ] both have the default configuration distributed
* [ ] both have same multicast group to share configuration events
* [ ] both use openvocs user administration (LDAP = false)

### Login
* [ ] open page
* [ ] -> login mask is displayed
* [ ] login with admin - admin
* [ ] -> login successful
* [ ] logout
* [ ] -> login mask is displayed, no error message
* [ ] login with admin - no
* [ ] -> error message: login failed
* [ ] login with admin - admin
* [ ] -> login successful

### Sharing of information between servers

### Prime offline

#### During configuration

#### Before login

### Backup offline

#### During configuration

#### Before login

### Different configurations

## Part 2: LDAP users

### Prerequisites
* [ ] 2 openvocs servers (main & backup)
* [ ] both have the default configuration distributed
* [ ] both have same multicast group to share configuration events
* [ ] both are configured for LDAP user import

## Part 3: LDAP roles

### Prerequisites
* [ ] 2 openvocs servers (main & backup)
* [ ] both have the default configuration distributed
* [ ] both have same multicast group to share configuration events
* [ ] both are configured for LDAP user and role import

## Offline mode

* [ ] open existing project
* [ ] save project is enabled 
* [ ] trigger offline mode (e.g. disable server)
* [ ] save project is disabled
* [ ] dismiss offline mode (e.g. restart server)
* [ ] save project is enabled

## Save new project without ID set

* [ ] create new project
* [ ] don't set ID
* [ ] save project
* [ ] error notice should be displayed