project = {
    id: "D1",
    domain: "openvocs.org",
    name: "openvocs Demo Project",
    logo_url: "none"
};

users = createUsers(120);
roles = createRoles(20);
loops = createLoops(88);

//console.log(JSON.stringify({ project: project, users: users, roles: roles, loops: loops }));

function createUsers(number) {
    let users = [];
    let userNamesF = ["Anna", "Julia", "Sara", "Lisa", "Laura", "Katharina", "Vanessa", "Lena", "Jennifer", "Anika",
        "Jana", "Lea", "Marie", "Jasmin", "Jessika", "Franziska", "Michelle", "Nina", "Hanna", "Alina",
        "Johanna", "Kim", "Nadine", "Jaqueline", "Caroline", "Christina", "Ann", "Svenja", "Luisa", "Natalie",
        "Sophie", "Lara", "Sandra", "Janina", "Sabrina", "Melanie", "Isabel", "Antonia", "Leoni", "Saskia",
        "Denise", "Eileen", "Nele", "Stefanie", "Pia", "Melina", "Sina", "Janin", "Vivien", "Sophia",
        "Alexandra", "Katrin", "Rebecca", "Lina", "Josephine", "Anne", "Merle", "Miriam", "Mareike", "Nicole",
        "Paula", "Celina", "Maria", "Viktoria", "Clara", "Charlotte", "Maike", "Larissa", "Kimberly", "Carina",
        "Linda", "Christin", "Jenny", "Celine", "Daniela", "Melissa", "Jule", "Patricia", "Ronja", "Elena",
        "Kira", "Sonja", "Gina", "Bianka", "Theresa", "Yvonne", "Emily", "Tanja", "Nora", "Lynn",
        "Pauline", "Paulina", "Svea", "Natascha", "Marina", "Angelina", "Friederike", "Nadja", "Alicia", "Lilly",
        "Juliane", "Eva", "Anja", "Carlotta", "Amelie", "Chantale", "Maja", "Milena", "Katja", "Chiara",
        "Annabell", "Mona", "Karla", "Marlene", "Alena", "Joana", "Claudia", "Tatjana", "Inga", "Emma",
        "Selina", "Luise", "Fiona", "Finja", "Verena", "Jill", "Janne", "Samantha", "Frederike", "Helena",
        "Fenja", "Elisabeth", "Cindy", "Tina", "Tamara", "Annalena", "Judith", "Christine", "Henrike", "Mia",
        "Wiebke", "Ricarda", "Stella", "Maren", "Helen", "Janna", "Ina", "Monique", "Diana", "Elisa",
        "Sandy", "Svantje", "Karolina", "Malin", "Birte", "Joanna", "Esther", "Mara", "Valerie", "Dana",
        "Julie", "Samira", "Fabienne", "Madeleine", "Andrea", "Simone", "Zoé", "Lucy", "Marleen", "Angelique",
        "Jasmina", "Greta", "Tabea", "Ramona", "Sabine", "Magdalena", "Dominique", "Malina", "Tessa",
        "Charline", "Kaja", "Corinna", "Karen", "Alica", "Lydia", "Jannika", "Yasemin", "Liza", "Maxi",
        "Michaela", "Annkatrin", "Susanne", "Kerstin", "Mira", "Alice", "Dilara", "Emilia", "Olivia", "Mariam"]
    let userNamesM = ["Jan", "Lukas", "Philip", "Tim", "Alexander", "Daniel", "Tobias", "Kevin", "Marcel", "Denis",
        "Felix", "Florian", "Niklas", "Maximilian", "Patrick", "Fabian", "Jonas", "Sebastian", "Christian", "Yannik",
        "Julian", "Marvin", "Timo", "David", "Nico", "Mark", "Finn", "Leon", "Lennard", "Max",
        "Tom", "Nils", "Pascal", "Simon", "Dominik", "Christopher", "Benjamin", "Moritz", "Lars", "Paul",
        "Marco", "Sven", "Robin", "Christoph", "Malte", "Torben", "Johannes", "Mike", "Jakob", "Michael",
        "Martin", "André", "Stefan", "Matthias", "Hendrik", "Björn", "Luca", "Nikolas", "Steven", "René",
        "Vincent", "Sascha", "Jonathan", "Markus", "Frederik", "Thomas", "Robert", "Erik", "Oliver", "Nick",
        "Ole", "Marius", "Justin", "Joshua", "Andreas", "Lasse", "Kai", "Maurice", "Manuel", "Konstantin",
        "Luis", "Adrian", "Steffen", "Mirko", "Anton", "Benedikt", "John", "Julius", "Til", "Jannis",
        "Henry", "Henrik", "Bastian", "Sören", "Leonard", "Johann", "Michel", "Hannes", "Mohammed", "Rafael",
        "Karl", "Ben", "Arne", "Viktor", "Mario", "Dustin", "Nikolai", "Linus", "Toni", "Marlon",
        "Valentin", "Peter", "Danny", "Jasper", "Julien", "Joel", "Aaron", "Henning", "Klemens", "Justus",
        "Kim", "Chris", "Hauke", "Thore", "Pierre", "Gerrit", "Mats", "Cedrik", "Bennet", "Riccardo",
        "Jannek", "Fabio", "Melvin", "Leo", "Karsten", "Jens", "Richard", "Leif", "Artur", "Ali",
        "Peer", "Samuel", "Deniz", "Jean", "Timon", "Noah", "Hans", "Kilian", "Eike", "Jannes",
        "Jeremy", "Calvin", "Marten", "Merlin", "Ahmed", "Elias", "Phil", "Dario", "Can", "Morten",
        "Lorenz", "Jason", "Georg", "Jona", "Oskar", "Enrico", "Konrad", "Tristan", "Armin", "Colin",
        "Sönke", "Klaas", "Cem", "Rico", "Alex", "Thies", "Brian", "Levin", "Thorge", "Kornelius",
        "Matthis", "Ralf", "Norman", "Steve", "Emre", "Gabriel", "Ruben", "Sandro", "Mehmet", "Ahmad",
        "Frank", "Jerome", "Gregor", "Marek", "Torsten", "Bjarne", "Tilo", "Roman"]
    let userLastName = ["A.", "B.", "C.", "D.", "F.", "G.", "H.", "I.", "J.", "K.",
        "L.", "M.", "N.", "O.", "P.", "R.", "S.", "T.", "U.", "V.", "W."];
    try {
        for (let index = 0; index < number; index++) {
            let indexLastName = index % userLastName.length;

            if (index % 2 === 0)
                users.push(newUser(index, userNamesF[index / 2], userLastName[indexLastName]));
            else
                users.push(newUser(index, userNamesM[(index - 1) / 2], userLastName[indexLastName]));
        }
    } catch (e) {
        console.error("max. number of users 398", e);
    }
    return users;
}

function createRoles(number) {
    let roles = [];
    let roleNames = ["Flight Director", "FD",
        "Spacecraft Communicator", "CAPCOM",
        "Flight Dynamics Officer", "FDO",
        "Guidance Procedures Officer", "GPO",
        "Data Processing System (Engineer)", "DPS",
        "Surgeon", "Surgeon",
        "Booster Engineer", "Booster",
        "Payload Deploy Retrieval System", "PDRS",
        "Propulsion Engineer", "PROP",
        "Guidance, Navigation and Controls System Engineer", "GNC",
        "Electrical, Environmental and Consumables Manager", "EECOM",
        "Electrical Generation and Illumination Engineer", "EGIL",
        "Instrumentation and Communications Officer", "INCO",
        "Russian Interface Operator", "RIO",
        "Ground Controller", "GC",
        "Flight Activities Officer", "FAO",
        "Payloads Officer", "PAYLOAD",
        "Maintenance, Mechanical, Arm and Crew Systems", "MMACS",
        "Public Affairs Officer", "PAO",
        "Mission Operations Directorate Manager", "MOD"];

    try {
        for (let index = 0; index < number; index++) {
            let roleIndex = index * 2;
            let roleUsers = randomEntries(users, randomInt(4, 10), "id")
            roles.push(newRole(index, roleNames[roleIndex], roleNames[roleIndex + 1], roleUsers));
        }
    } catch (e) {
        console.error("max. number of roles 20", e);
    }
    return roles;
}

function createLoops(number) {
    let loops = [];
    let starNames = [{
        "abbr": "And",
        "name": "Andromeda",
        "genitive": "Andromedae",
        "en": "Andromeda (mythological character)"
    }, {
        "abbr": "Ant",
        "name": "Antlia",
        "genitive": "Antliae",
        "en": "air pump"
    }, {
        "abbr": "Aps",
        "name": "Apus",
        "genitive": "Apodis",
        "en": "Bird-of-paradise"
    }, {
        "abbr": "Aqr",
        "name": "Aquarius",
        "genitive": "Aquarii",
        "en": "water-bearer"
    }, {
        "abbr": "Aql",
        "name": "Aquila",
        "genitive": "Aquilae",
        "en": "eagle"
    }, {
        "abbr": "Ara",
        "name": "Ara",
        "genitive": "Arae",
        "en": "altar"
    }, {
        "abbr": "Ari",
        "name": "Aries",
        "genitive": "Arietis",
        "en": "ram"
    }, {
        "abbr": "Aur",
        "name": "Auriga",
        "genitive": "Aurigae",
        "en": "charioteer"
    }, {
        "abbr": "Boo",
        "name": "Bo\u00f6tes",
        "genitive": "Bo\u00f6tis",
        "en": "herdsman"
    }, {
        "abbr": "Cae",
        "name": "Caelum",
        "genitive": "Caeli",
        "en": "chisel or graving tool"
    }, {
        "abbr": "Cam",
        "name": "Camelopardalis",
        "genitive": "Camelopardalis",
        "en": "giraffe"
    }, {
        "abbr": "Cnc",
        "name": "Cancer",
        "genitive": "Cancri",
        "en": "crab"
    }, {
        "abbr": "CVn",
        "name": "Canes Venatici",
        "genitive": "Canum Venaticorum",
        "en": "hunting dogs"
    }, {
        "abbr": "CMa",
        "name": "Canis Major",
        "genitive": "Canis Majoris",
        "en": "greater dog"
    }, {
        "abbr": "CMi",
        "name": "Canis Minor",
        "genitive": "Canis Minoris",
        "en": "lesser dog"
    }, {
        "abbr": "Cap",
        "name": "Capricornus",
        "genitive": "Capricorni",
        "en": "sea goat"
    }, {
        "abbr": "Car",
        "name": "Carina",
        "genitive": "Carinae",
        "en": "keel"
    }, {
        "abbr": "Cas",
        "name": "Cassiopeia",
        "genitive": "Cassiopeiae",
        "en": "Cassiopeia (mythological character)"
    }, {
        "abbr": "Cen",
        "name": "Centaurus",
        "genitive": "Centauri",
        "en": "centaur"
    }, {
        "abbr": "Cep",
        "name": "Cepheus",
        "genitive": "Cephei",
        "en": "Cepheus (mythological character)"
    }, {
        "abbr": "Cet",
        "name": "Cetus",
        "genitive": "Ceti",
        "en": "sea monster (whale)"
    }, {
        "abbr": "Cha",
        "name": "Chamaeleon",
        "genitive": "Chamaeleontis",
        "en": "chameleon"
    }, {
        "abbr": "Cir",
        "name": "Circinus",
        "genitive": "Circini",
        "en": "compasses"
    }, {
        "abbr": "Col",
        "name": "Columba",
        "genitive": "Columbae",
        "en": "dove"
    }, {
        "abbr": "Com",
        "name": "Coma Berenices",
        "genitive": "Comae Berenices",
        "en": "Berenice's hair"
    }, {
        "abbr": "CrA",
        "name": "Corona Australis",
        "genitive": "Coronae Australis",
        "en": "southern crown"
    }, {
        "abbr": "CrB",
        "name": "Corona Borealis",
        "genitive": "Coronae Borealis",
        "en": "northern crown"
    }, {
        "abbr": "Crv",
        "name": "Corvus",
        "genitive": "Corvi",
        "en": "crow"
    }, {
        "abbr": "Crt",
        "name": "Crater",
        "genitive": "Crateris",
        "en": "cup"
    }, {
        "abbr": "Cru",
        "name": "Crux",
        "genitive": "Crucis",
        "en": "southern cross"
    }, {
        "abbr": "Cyg",
        "name": "Cygnus",
        "genitive": "Cygni",
        "en": "swan or Northern Cross"
    }, {
        "abbr": "Del",
        "name": "Delphinus",
        "genitive": "Delphini",
        "en": "dolphin"
    }, {
        "abbr": "Dor",
        "name": "Dorado",
        "genitive": "Doradus",
        "en": "dolphinfish"
    }, {
        "abbr": "Dra",
        "name": "Draco",
        "genitive": "Draconis",
        "en": "dragon"
    }, {
        "abbr": "Equ",
        "name": "Equuleus",
        "genitive": "Equulei",
        "en": "pony"
    }, {
        "abbr": "Eri",
        "name": "Eridanus",
        "genitive": "Eridani",
        "en": "river Eridanus (mythology)"
    }, {
        "abbr": "For",
        "name": "Fornax",
        "genitive": "Fornacis",
        "en": "chemical furnace"
    }, {
        "abbr": "Gem",
        "name": "Gemini",
        "genitive": "Geminorum",
        "en": "twins"
    }, {
        "abbr": "Gru",
        "name": "Grus",
        "genitive": "Gruis",
        "en": "Crane"
    }, {
        "abbr": "Her",
        "name": "Hercules",
        "genitive": "Herculis",
        "en": "Hercules (mythological character)"
    }, {
        "abbr": "Hor",
        "name": "Horologium",
        "genitive": "Horologii",
        "en": "pendulum clock"
    }, {
        "abbr": "Hya",
        "name": "Hydra",
        "genitive": "Hydrae",
        "en": "Hydra (mythological creature)"
    }, {
        "abbr": "Hyi",
        "name": "Hydrus",
        "genitive": "Hydri",
        "en": "lesser water snake"
    }, {
        "abbr": "Ind",
        "name": "Indus",
        "genitive": "Indi",
        "en": "Indian"
    }, {
        "abbr": "Lac",
        "name": "Lacerta",
        "genitive": "Lacertae",
        "en": "lizard"
    }, {
        "abbr": "Leo",
        "name": "Leo",
        "genitive": "Leonis",
        "en": "lion"
    }, {
        "abbr": "LMi",
        "name": "Leo Minor",
        "genitive": "Leonis Minoris",
        "en": "lesser lion"
    }, {
        "abbr": "Lep",
        "name": "Lepus",
        "genitive": "Leporis",
        "en": "hare"
    }, {
        "abbr": "Lib",
        "name": "Libra",
        "genitive": "Librae",
        "en": "balance"
    }, {
        "abbr": "Lup",
        "name": "Lupus",
        "genitive": "Lupi",
        "en": "wolf"
    }, {
        "abbr": "Lyn",
        "name": "Lynx",
        "genitive": "Lyncis",
        "en": "lynx"
    }, {
        "abbr": "Lyr",
        "name": "Lyra",
        "genitive": "Lyrae",
        "en": "lyre / harp"
    }, {
        "abbr": "Men",
        "name": "Mensa",
        "genitive": "Mensae",
        "en": "Table Mountain (South Africa)"
    }, {
        "abbr": "Mic",
        "name": "Microscopium",
        "genitive": "Microscopii",
        "en": "microscope"
    }, {
        "abbr": "Mon",
        "name": "Monoceros",
        "genitive": "Monocerotis",
        "en": "unicorn"
    }, {
        "abbr": "Mus",
        "name": "Musca",
        "genitive": "Muscae",
        "en": "fly"
    }, {
        "abbr": "Nor",
        "name": "Norma",
        "genitive": "Normae",
        "en": "carpenter's level"
    }, {
        "abbr": "Oct",
        "name": "Octans",
        "genitive": "Octantis",
        "en": "octant (instrument)"
    }, {
        "abbr": "Oph",
        "name": "Ophiuchus",
        "genitive": "Ophiuchi",
        "en": "serpent-bearer"
    }, {
        "abbr": "Ori",
        "name": "Orion",
        "genitive": "Orionis",
        "en": "Orion (mythological character)"
    }, {
        "abbr": "Pav",
        "name": "Pavo",
        "genitive": "Pavonis",
        "en": "peacock"
    }, {
        "abbr": "Peg",
        "name": "Pegasus",
        "genitive": "Pegasi",
        "en": "Pegasus (mythological creature)"
    }, {
        "abbr": "Per",
        "name": "Perseus",
        "genitive": "Persei",
        "en": "Perseus (mythological character)"
    }, {
        "abbr": "Phe",
        "name": "Phoenix",
        "genitive": "Phoenicis",
        "en": "phoenix"
    }, {
        "abbr": "Pic",
        "name": "Pictor",
        "genitive": "Pictoris",
        "en": "easel"
    }, {
        "abbr": "Psc",
        "name": "Pisces",
        "genitive": "Piscium",
        "en": "fishes"
    }, {
        "abbr": "PsA",
        "name": "Piscis Austrinus",
        "genitive": "Piscis Austrini",
        "en": "southern fish"
    }, {
        "abbr": "Pup",
        "name": "Puppis",
        "genitive": "Puppis",
        "en": "poop deck"
    }, {
        "abbr": "Pyx",
        "name": "Pyxis",
        "genitive": "Pyxidis",
        "en": "mariner's compass"
    }, {
        "abbr": "Ret",
        "name": "Reticulum",
        "genitive": "Reticuli",
        "en": "eyepiece graticule"
    }, {
        "abbr": "Sge",
        "name": "Sagitta",
        "genitive": "Sagittae",
        "en": "arrow"
    }, {
        "abbr": "Sgr",
        "name": "Sagittarius",
        "genitive": "Sagittarii",
        "en": "archer"
    }, {
        "abbr": "Sco",
        "name": "Scorpius",
        "genitive": "Scorpii",
        "en": "scorpion"
    }, {
        "abbr": "Scl",
        "name": "Sculptor",
        "genitive": "Sculptoris",
        "en": "sculptor"
    }, {
        "abbr": "Sct",
        "name": "Scutum",
        "genitive": "Scuti",
        "en": "shield (of Sobieski)"
    }, {
        "abbr": "Ser",
        "name": "Serpens",
        "genitive": "Serpentis",
        "en": "snake"
    }, {
        "abbr": "Sex",
        "name": "Sextans",
        "genitive": "Sextantis",
        "en": "sextant"
    }, {
        "abbr": "Tau",
        "name": "Taurus",
        "genitive": "Tauri",
        "en": "bull"
    }, {
        "abbr": "Tel",
        "name": "Telescopium",
        "genitive": "Telescopii",
        "en": "telescope"
    }, {
        "abbr": "Tri",
        "name": "Triangulum",
        "genitive": "Trianguli",
        "en": "triangle"
    }, {
        "abbr": "TrA",
        "name": "Triangulum Australe",
        "genitive": "Trianguli Australis",
        "en": "southern triangle"
    }, {
        "abbr": "Tuc",
        "name": "Tucana",
        "genitive": "Tucanae",
        "en": "toucan"
    }, {
        "abbr": "UMa",
        "name": "Ursa Major",
        "genitive": "Ursae Majoris",
        "en": "great bear"
    }, {
        "abbr": "UMi",
        "name": "Ursa Minor",
        "genitive": "Ursae Minoris",
        "en": "lesser bear"
    }, {
        "abbr": "Vel",
        "name": "Vela",
        "genitive": "Velorum",
        "en": "sails"
    }, {
        "abbr": "Vir",
        "name": "Virgo",
        "genitive": "Virginis",
        "en": "virgin or maiden"
    }, {
        "abbr": "Vol",
        "name": "Volans",
        "genitive": "Volantis",
        "en": "flying fish"
    }, {
        "abbr": "Vul",
        "name": "Vulpecula",
        "genitive": "Vulpeculae",
        "en": "fox"
    }];

    try {
        for (let index = 0; index < number; index++) {
            let monitorRoles = randomEntries(roles, randomInt(2, roles.length), "id");
            let talkRoles = randomEntries(roles, randomInt(2, roles.length), "id");
            loops.push(newLoop(index, starNames[index].name, starNames[index].abbr, monitorRoles, talkRoles));
        }
    } catch (e) {
        console.error("max. number of loops 88", e);
    }
    return loops;
}

function randomInt(min, max) {
    min = Math.ceil(min);
    max = Math.floor(max);
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function randomEntries(array, numberEntries, id) {
    console.log(id, numberEntries);
    let entries = [];
    for (let index = 0; index < numberEntries; index++) {
        let item = array[Math.floor(Math.random() * array.length)];
        if (entries.includes(item[id])) {
            console.log("here");
            index--;
        } else {
            entries.push(item[id]);
        }
    }
    return entries
}

function newUser(nr, firstName, secondName) {
    user = {
        id: "U" + nr,
        name: firstName + " " + secondName,
        avatar: "none",
        owner: project.id
    }
    return user;
}

function newRole(nr, name, abr, roleUsers) {
    role = {
        id: "R" + nr,
        name: name,
        abbreviation: abr,
        color: "none",
        users: roleUsers,
        owner: project.id
    }
    return role;
}

function newLoop(nr, name, abr, monitor, talk) {
    loop = {
        id: "L" + nr,
        type: "audio",
        name: name,
        abbreviation: abr,
        talk: talk,
        monitor: monitor,
        owner: project.id
    }
    return loop;
}