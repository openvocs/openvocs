<!--
SPDX-FileCopyrightText: 2025 German Aerospace Center (DLR)
SPDX-License-Identifier: Apache 2.0
-->
<div style="text-align: center;">
<img src="[Images](https://github.com/openvocs/openvocs/blob/main/Handbook_DRAFT/images)/ov black.png"" alt="OV_LOGI" title="Total Time Human in Space" />

# **O P E N V O C S ®**
## User Guide v.01.3 - Last Update 2025-06-30 - Status: ***DRAFT***
</div>

---
## Table of Content
<!--* -->
* [1. Introduction](#Introduction)
* [2. OPENVOCS](#OPENVOCS)
* [3. Usage](#usage)
    * [3.1 Voice Client](#usagevoice)
    * [3.2 Admin Client](#usageadmin)
* [4. Contributors](#Contributors)
* [5. Contact](#Contact)

---
## [1. Introduction][toc]
Voice communication is a crucial part to successfully conduct a space missions.
To have an voice system, that is able to provide all necessary functionality is highly demanded. That is the reason the German Aerospace Center (Deutsches Zentrum für Luft und Raumfahrt - DLR e.V.)
have started the project "OPENVOCS®" to develop a new dynamic voice conferencing system.

---
## [2. OPENVOCS®][toc]
OPENVOCS® is a pure software-based voice communication system developed within the department 'Space Operations and Astronaut Training'. It was developed especially for space mission operations. Therefore, the voice communication is organized via voice loops (multiple audio conferences held in parallel). Further, the system provides a role-based access control, where depended on specific user roles, the access to voice loops is restricted. Moreover, the allowance to only monitor or talk in a voice loop is differentiated. The development focuses on flexibility an scalability. Therefore, the system uses common internet technology. The underlying system concept is a simple server-client structure. The voice interface can operate from within a standard internet browser (**Note** Firefox v.?? is tested and recommended, other browser also work, may lead to unexpected behavior).

---
### [3. Usage][toc]
In the following the usage of OPENVOCS® is shown. This section starts with explaining the voice client and is followed by explaining the administration interface.

### [3.1 Voice Client][toc]

### Login and role selection
- The voice interface is reachable under "server-IP/openvocs/vocs".
- Login website => enter username and password to authenticate, click LOGIN-button to continue.
- By clicking the circle arrow symbol, one can reload the login-page.

<img src="Images/01_Login.png" alt="figure_not_found" width="40%"/>

- Role selection => select the role or project by clicking on it (only the roles and projects are shown that are allowed for the user).
- The voice loops associated with the role/project and the corresponding layout is loaded automatically.

<img src="Images/02_Role_Selection.png" alt="RolesProjects" width="40%"/>

- After the role is selceted the voice client will start automatically.
- **Note**: The role can be switched later in directly in the side menue, shown later in this guide.

### User interface - voice client
- Start of voice client GUI (switched off - all tiles are in grey color).
- If all tiles are grey, even if there is voice traffic, no audio is audible.
- It can be possible that there are more then one page with voice loop tiles.
- The page selection is placed on the lower-right corner and numbered.


<img src="Images/03_Loop_off.png" alt="GUI_VL_OFF" width="70%"/>

---
#### ***Voice client - tile elements***
<img src="Images/03_GUI_OFF_Kachel_new.png" alt="figure_not_found" width="40%"/>

- The name or abbreviation of the voice loop is placed in the middle of the tile.
- Headphones and microphone symbols indicates allowance in a loop.
- Headphones only => only monitoring of the loop is allowed.
- Headphones and microphone => talking is also allowed in this loop.
- Icon with number => shows the number of active users in the loop.
- These areas are used to switch mode of the tile (see next section).
- The "OFF" button is dedicated to switch off this specific voice loop, and ends the audio transmission.
- Speaker icon opens a slider => change the volume of the loop.
- **Note**: Telephony Panel – is deactivated in the opensource version of OPENVOCS®.


##### Change the mode from "monitoring" to "talk"
- 1st click on the tile (in grey OFF-state) will activate the monitoring mode first, indicated via blue color.
- 2nd click on the tile will activate the talking mode, indicated via green color.
- 3rd click on the tile will activate the monitoring mode again (blue color).
- To switch the loop OFF (back to grey color), click on the OFF-Button in the right corner.
- It is possible to select several voice loops for monitoring mode, but only one for talking mode.
- If a new voice loop is selected for talking mode, the former voice loop will switch back to monitoring automatically.
- **Note**: If a tile only have a headphones symbol, the talking mode is not accessible.

---
#### ***Voice client (monitoring: blue / talking: green -- switched ON)***
<img src="Images/04_Loop_Talk.png" alt="figure_not_found" width="70%"/>

#### ***Voice client (talking: green -- PTT-bar switched ON)***
- To speak in a voice loop, the voice loop tile needs to be in talking mode (green color).
- In addition the PTT bar needs to be pressed.
- Three types of conformation about an open microphone are given:
  - 1. The PTT-bar will change to green color when pressed to indicate that talking is activated.
  - 2. A small audio waveform will be shown according to the microphone signal.
  - 3. The name/role of the user, that has an open microphone will appear under the tile name.
  - 4. The white frame indicates voice traffic on a voice loop.

<img src="Images/05_active-waveform.png" alt="figure_not_found" width="70%"/>

- **Note**: Depending on the configuration of OPENVOCS®, a screen PTT is also available, or only the PTT on the headset can be used.


#### ***Voice client (volume slider -- 0-100 => 50 default)***
- Volume control panel, after touching the speaker symbol, the volume slider shows up and the bar can be moved up (louder) and down (quieter), the number on the side indicated the volume slider value:

    - 0 – no sound at all
    - 100 – maximal loudness

-To close the volume slider, one have to touch the speaker symbol again.

<img src="Images/06_GUI_Vol_Slider_new.png" alt="figure_not_found" width="40%"/>

#### ***Voice client (menue opened via clicking on the OPENVOCS® logo)***
- On top the user name is depicted.
- Below that the role/project that is currently chosen is shown.
- In the server panel, all available OPENVOCS® server are shown. The server marked blue, is the server currently connected. The green dot, indicates the availability of the server. Is this indicator red, the server is not available.
- Further, a page reload button (circle arrows symbol) and additional options can be found (gear symbol).
- At the bottom is a red logout-button located, clicking on it, will disconnect and close the voice client GUI, leading back to the login page.

<img src="Images/07_menu-open.png" alt="figure_not_found" width="70%"/>


### [3.2 Admin Client][toc]
#### Setup and configure a project/missions
- The admin interface is reachable under "server-IP/openvocs/admin".
- After login the administration interface starts with a selection of a project (e.g. missions).
  - The default admin login is:
	```
	lg: admin
	ps: admin
	```
- The list of existing project will be shown.
- To administrate a project simply choose the project be clicking on it.
- For creating a new project, the "**+**" icon is selected.
- The gear wheel in the top-right corner is for additional options.

<img src="Images/01_project_select.png" alt="placeholder" width="70%"/>

- After selecting a project, the administration interface with all available options opens.
- On this page one can also delete the project, by opening the "delete" menue.
- Changes can be saved via the "Save" button place at the bottom.

<img src="Images/02_project_open.png" alt="placeholder" width="70%"/>


#### Setup: Role - Voice Loop - User
- To configure, add, delete or change anything according to roles, voice loops and user one open via access management, on the panel at the bottom.
- **Note**: Depending on the version of OPENVOCS®, a SIP and RECORDER setup option are available.

<img src="Images/03_role_vl_user.png" alt="placeholder" width="70%"/>

- Here all users, roles and voice loops for that project are shown.
- Moreover the connection between users, their roles and available voice loops are graphically indicated.
- For a better overview, one can hide unused users, roles, and voice loop, for that project, via the options hidden under the hamburger menue on the top-left corner.
- One can add new users, roles and voice loops by clicking on each "**+**" symbol

<img src="Images/03_role_options.png" alt="placeholder" width="70%"/>

- For changes on a user, role, or voice loop, click on the gear symbol, that appears when hoovering over it.
- In addition a linking / delinking symbol will appear.
- By clicking on it, one can create a link between as user and role, and link a role to a set of voice loops.
- The color will change to yellow, when selected.
- To exit the selection mode, one clicks on the door-exit symbol on the selected panel.

<img src="Images/03_role_vl_user_link_01.png" alt="placeholder" width="70%"/>

<img src="Images/03_role_vl_user_link_02.png" alt="placeholder" width="70%"/>

<img src="Images/03_role_vl_user_link_03.png" alt="placeholder" width="70%"/>


#### Setup voice loop layout
- After selecting layout from the bottom panel, the layout setup  starts.
- Under the burger menue on upper-left, general layout options are placed.
        - Number of packages
        - Scale
        - Layout tpy (grid or list)
        - Columns and rows
        - Name and font scaling

<img src="Images/06_layout_grid_option.png" alt="placeholder" width="70%"/>

---
- In the drop-down menue at the top, the layout for specific roles in the project can be selected.
- After selecting on role the current layout for that role is loaded.
- **Note**: When starting a new project, the grid will be empty.

<img src="Images/04_layout_01.png" alt="placeholder" width="70%"/>

<img src="Images/05_layout_grid.png" alt="placeholder" width="70%"/>

- To add a new voice loop to the grid or change an existing voice loop position, one select on tile.
- The tile will change to a green frame, to indicate the selection.
- In addition a drop-down menue will appear.
- From that menue, a voice loop can be selected.
- **Note**: Only the voice loops that are linked before for that role will be available.
- The selection of the tile will end automatically when a new voice loop was selected or when clicking on the surrounding areas around the drop-down menue.


#### Import and Export Configurations
- When clicking on the OPENVOCS® logo in the lower-left corner one can find the options to import or export saved configurations.
- Via import, are previously saved configuration file (json) can be loaded-
- Via export, a json-file is directly download to the download folder on the system.
- The back button brings one back to the project selection page.
- Further, the logout button in located there.

<img src="Images/07_admin_imexport.png" alt="placeholder" width="70%"/>

#### [LDAP import][toc]
- With this option one can import data from LDAP.
<img src="Images/08_LDAP_import.png" alt="placeholder" width="70%"/>

---

## [4. Contributors][toc]
Here you find the main contributors to the material:

- Markus Töpfer
- Michael Beer
- Anja Bertard
- Tobias Kolb
- Falk Schiffner

## [5. Contact][toc]
http://openvocs.org

---


[toc]: #table-of-content "go to Table of Content"
