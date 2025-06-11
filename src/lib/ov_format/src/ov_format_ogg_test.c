/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_format_ogg.c"
#include <ov_base/ov_string.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int test_ov_format_ogg_install() {

    testrun(ov_format_ogg_install(0));
    ov_format_registry_clear(0);

    ov_format_registry *registry = ov_format_registry_create();
    testrun(0 != registry);

    testrun(ov_format_ogg_install(registry));
    registry = ov_format_registry_clear(registry);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ssize_t write_chunk_to(ov_format *oggf, char const *str) {

    size_t len = strlen(str);

    ov_buffer *buf = ov_buffer_create(len);
    ov_buffer_set(buf, str, len);

    ssize_t written_octets = ov_format_payload_write_chunk(oggf, buf);

    buf = ov_buffer_free(buf);

    return written_octets;
}

/*----------------------------------------------------------------------------*/

static uint8_t const *skip_to_next_page(uint8_t const *current_page,
                                        uint8_t const *mem,
                                        size_t *memlen_bytes) {

    if ((0 == current_page) || (0 == memlen_bytes) || (0 == *memlen_bytes)) {
        return 0;
    } else {

        uint8_t no_segments = current_page[26];

        size_t payload_length = no_segments;

        for (uint8_t segment = 0; segment < no_segments; ++segment) {

            payload_length += current_page[27 + segment];
        }

        size_t bytes_to_skip = payload_length + 27;

        fprintf(stderr, "Skipping %zu bytes to next page\n", bytes_to_skip);

        current_page += bytes_to_skip;

        if (current_page >= mem + *memlen_bytes) {

            // We reached the end of our mem block and hence input data
            return 0;
        } else {
            *memlen_bytes = -payload_length + 27;
            return current_page;
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool check_ogg_integrity(uint8_t const *mem, size_t memlen_bytes) {

    uint8_t const *last_page = mem;
    uint8_t const *current_page = mem;

    // First page MUST have 'begin of stream' set
    // MUST NOT have 'continuation' set
    // Might have 'end of stream' set if there is only one page
    if ((0x02 != (0x02 & current_page[5])) ||
        (0x01 == (0x01 & current_page[5]))) {
        return false;
    }

    size_t bytes_remaining = memlen_bytes;

    current_page = skip_to_next_page(current_page, mem, &bytes_remaining);

    while (current_page) {

        if (0 != memcmp(current_page, "OggS", 4)) {
            return false;
        }

        last_page = current_page;
        current_page = skip_to_next_page(current_page, mem, &bytes_remaining);

        if ((0 != current_page) && (0 != (0x04 & last_page[5]))) {
            return false;
        }
    }

    return (0 != last_page) && (0x04 == (0x04 & last_page[5]));
}

/*----------------------------------------------------------------------------*/

int test_impl_write_chunk() {

    ov_format_ogg_install(0);

    ov_format_ogg_options options = {

        .chunk_length_ms = 20,
        .samplerate_hz = 48000,

    };

    char const *fname = "/tmp/test_impl_chunk.ogg";
    ov_format *lower_format = ov_format_open(fname, OV_WRITE);
    testrun(0 != lower_format);

    ov_format *ogg = ov_format_as(lower_format, "ogg", &options, 0);

    write_chunk_to(ogg, "Eins zwei drei vier...\n");
    write_chunk_to(ogg, "Valgame dios!\n");
    char const *chunk1 =
        "Tosten Sie den Toast bitte nicht ganz durch."
        "er gaehrt gern gut und grummelig. Das ist seine Bestimmung."
        "Des Toasts Schicksal ist der Verzehr desselben. Daher "
        "sollte er"
        "stets mit Genuss genug Geschehen ueber sich erGehen lassen."
        "Falls der Sinn dieser Zeilen sich nicht unmittelbar "
        "erschliesst -"
        "Sie sind nicht der einzige. Vielleicht waren "
        "Betaeubungsmittel"
        "im Spiel?\n";

    char const *chunk2 =
        "1\n"
        "Allen Edeln   gebiet ich Andacht,\n"
        "\n"
        "Hohen und Niedern   von Heimdalls Geschlecht;\n"
        "Ich will Walvaters   Wirken künden,\n"
        "Die ältesten Sagen,   der ich mich entsinne.\n"
        "\n"
        "2\n"
        "Riesen acht ich   die Urgebornen,\n"
        "\n"
        "Die mich vor Zeiten   erzogen haben.\n"
        "Neun Welten kenn ich,   neun Äste weiß ich\n"
        "An dem starken Stamm15   im Staub der Erde.\n"
        "\n"
        "3\n"
        "Einst war das Alter,   da Ymir4 lebte:\n"
        "\n"
        "Da war nicht Sand nicht See,   nicht salzge Wellen,\n"
        "Nicht Erde fand sich   noch Überhimmel,\n"
        "Gähnender Abgrund   und Gras nirgend.\n"
        "\n"
        "4\n"
        "Bis Börs Söhne8   die Bälle erhuben,\n"
        "\n"
        "Sie die das mächtige   Midgard schufen.\n"
        "Die Sonne von Süden   schien auf die Felsen\n"
        "Und dem Grund entgrünte   grüner Lauch.\n"
        "\n"
        "5\n";

    char const *chunk3 =
        "Die Sonne von Süden,   des Mondes Gesellin,\n"
        "\n"
        "Hielt mit der rechten Hand   die Himmelrosse.\n"
        "Sonne wuste nicht   wo sie Sitz hätte,\n"
        "Mond wuste nicht   was er Macht hätte,\n"
        "Die Sterne wusten nicht   wo sie Stätte hatten.\n"
        "[4]\n"
        "6\n"
        "Da14 gingen die Berather   zu den Richterstühlen,\n"
        "\n"
        "Hochheilge Götter   hielten Rath.\n"
        "Der Nacht und dem Neumond   gaben sie Namen,\n"
        "Hießen Morgen   und Mitte des Tags,\n"
        "Under und Abend,   die Zeiten zu ordnen.\n"
        "\n"
        "7\n"
        "Die Asen einten sich   auf dem Idafelde,\n"
        "\n"
        "Hof und Heiligtum   hoch sich zu wölben.14\n"
        "(Übten die Kräfte   Alles versuchend,)\n"
        "Erbauten Essen   und schmiedeten Erz,\n"
        "Schufen Zangen   und schön Gezäh.[WS 1]\n"
        "\n"
        "8\n"
        "Sie warfen im Hofe   heiter mit Würfeln\n"
        "\n"
        "Und darbten goldener   Dinge noch nicht.\n"
        "Bis drei der Thursen-   töchter kamen\n"
        "Reich an Macht,   aus Riesenheim.14\n"
        "\n"
        "9\n"
        "Da gingen die Berather   zu den Richterstühlen,\n"
        "\n"
        "Hochheilge Götter   hielten Rath,\n"
        "Wer schaffen sollte   der Zwerge Geschlecht\n"
        "Aus Brimirs Blut   und blauen Gliedern.\n"
        "\n"
        "10\n"
        "Da ward Modsognir   der mächtigste\n"
        "\n"
        "Dieser Zwerge   und Durin nach ihm.\n"
        "Noch manche machten sie   menschengleich\n"
        "Der Zwerge von Erde,   wie Durin angab.\n"
        "\n"
        "11\n"
        "Nyi und Nidi,   Nordri und Sudri,\n"
        "\n"
        "Austri und Westri,   Althiofr, Dwalin,\n"
        "Nar und Nain,   Nipingr, Dain,\n"
        "Bifur, Bafur,   Bömbur, Nori;\n"
        "Ann und Anarr,   Ai, Miödwitnir.\n"
        "\n"
        "12\n"
        "Weigr, Gandalfr,   Windalfr, Thrain,\n"
        "\n"
        "Theckr und Thorin,   Thror, Witr und Litr,\n"
        "Nar und Nyradr;   nun sind diese Zwerge,\n"
        "Regin und Raswidr,   richtig aufgezählt.\n"
        "[5]\n"
        "13\n"
        "Fili, Kili,   Fundin, Nali,\n"
        "\n"
        "Hepti, Wili,   Hannar und Swior,\n"
        "Billingr, Bruni,   Bildr, Buri,\n"
        "Frar, Hornbori,   Frägr und Loni,\n"
        "Aurwangr, Jari,   Eikinskjaldi.\n"
        "\n"
        "14\n"
        "Zeit ists, die Zwerge   von Dwalins Zunft\n"
        "\n"
        "Den Leuten zu leiten   bis Lofar hinauf,\n"
        "Die aus Gestein   und Klüften strebten\n"
        "Von Aurwangs Tiefen   zum Erdenfeld.\n"
        "\n"
        "15\n"
        "Da war Draupnir   und Dolgthrasir,\n"
        "\n"
        "Har, Haugspori,   Hläwangr, Gloi,\n"
        "Skirwir, Wirwir,   Skafidr, Ai,\n"
        "Alfr und Yngwi,   Eikinskjaldi.\n"
        "\n";

    char const *chunk4 =
        "16\n"
        "Fialar und Frosti,   Finnar und Ginnar,\n"
        "\n"
        "Heri, Höggstari,   Hliodolfr, Moin.\n"
        "So lange Menschen   leben auf Erden,\n"
        "Wird zu Lofar hinauf   ihr Geschlecht geleitet.\n"
        "\n"
        "17\n"
        "Gingen da9 dreie   aus dieser Versammlung,\n"
        "\n"
        "Mächtige, milde   Asen zumal,\n"
        "Fanden am Ufer   unmächtig\n"
        "Ask und Embla   und ohne Bestimmung.\n"
        "\n"
        "18\n"
        "Besaßen nicht Seele,   und Sinn noch nicht,\n"
        "\n"
        "Nicht Blut noch Bewegung,   noch blühende Farbe.\n"
        "Seele gab Odhin,   Hönir gab Sinn,\n"
        "Blut gab Lodur   und blühende Farbe.\n"
        "\n"
        "19\n"
        "Eine Esche weiß ich,   heißt Yggdrasil,15. 16\n"
        "\n"
        "Den hohen Baum netzt   weißer Nebel;\n"
        "Davon kommt der Thau,   der in die Thäler fällt.\n"
        "Immergrün steht er   über Urds Brunnen.\n"
        "\n"
        "20\n"
        "Davon15 kommen Frauen,   vielwißende,\n"
        "\n"
        "Drei aus dem See   dort unterm Wipfel.\n"
        "[6]\n"
        "\n"
        "Urd heißt die eine,   die andre Werdandi:\n"
        "Sie schnitten Stäbe;   Skuld hieß die dritte.\n"
        "Sie legten Looße,   das Leben bestimmten sie\n"
        "Den Geschlechtern der Menschen,   das Schicksal "
        "verkündend.\n"
        "\n"
        "21\n"
        "Allein saß sie außen,   da der Alte kam,\n"
        "\n"
        "Der grübelnde Ase,   und ihr ins Auge sah.\n"
        "Warum fragt ihr mich?   was erforscht ihr mich?\n"
        "Alles weiß ich, Odhin,   wo du dein Auge bargst:\n"
        "\n"
        "22\n"
        "In der vielbekannten   Quelle Mimirs.\n"
        "\n"
        "Meth trinkt Mimir   allmorgentlich\n"
        "Aus Walvaters Pfand!   wißt ihr was das bedeutet?15\n"
        "\n"
        "23\n"
        "Ihr gab Heervater   Halsband und Ringe\n"
        "\n"
        "Für goldene Sprüche   und spähenden Sinn.\n"
        "Denn weit und breit sah sie   über die Welten all.\n"
        "\n"
        "24\n"
        "Ich sah Walküren36   weither kommen,\n"
        "\n"
        "Bereit zu reiten   zum Rath der Götter.\n"
        "Skuld hielt den Schild,   Skögul war die andre,\n"
        "Gunn, Hilde, Göndul   und Geirskögul.\n"
        "Hier nun habt ihr   Herians Mädchen,\n"
        "Die als Walküren   die Welt durchreiten.\n"
        "\n"
        "25\n"
        "Da wurde Mord   in der Welt zuerst,\n"
        "\n"
        "Da sie mit Geeren   Gulweig (die Goldkraft) stießen,\n"
        "In des Hohen Halle   die helle brannten.\n"
        "Dreimal verbrannt   ist sie dreimal geboren,\n"
        "Oft, unselten,   doch ist sie am Leben.\n"
        "\n"
        "26\n"
        "Heid hieß man sie   wohin sie kam,\n"
        "\n"
        "Wohlredende Wala   zähmte sie Wölfe.\n"
        "Sudkunst konnte sie,   Seelenheil raubte sie,\n"
        "Übler Leute   Liebling allezeit.\n"
        "\n"
        "27\n"
        "Da42 gingen die Berather   zu den Richterstühlen,\n"
        "\n"
        "Hochheilge Götter   hielten Rath,\n"
        "[7]\n"
        "\n"
        "Ob die Asen sollten   Untreue strafen,\n"
        "Oder alle Götter   Sühnopfer empfahn.\n"
        "\n"
        "28\n"
        "Gebrochen war   der Burgwall den Asen,\n"
        "\n"
        "Schlachtkundge Wanen   stampften das Feld.\n"
        "Odhin schleuderte   über das Volk den Spieß:\n"
        "Da wurde Mord   in der Welt zuerst.\n"
        "\n"
        "29\n"
        "Da gingen die Berather   zu den Richterstühlen,\n"
        "\n"
        "Hochheilge Götter   hielten Rath,\n"
        "Wer mit Frevel hätte   die Luft erfüllt,\n"
        "Oder dem Riesenvolk   Odhurs Braut gegeben?\n"
        "\n"
        "30\n"
        "Von Zorn bezwungen   zögerte Thôr nicht,\n"
        "\n"
        "Er säumt selten   wo er Solches vernimmt:\n"
        "Da schwanden die Eide,   Wort und Schwüre,\n"
        "Alle festen Verträge   jüngst trefflich erdacht.\n"
        "\n"
        "31\n"
        "Ich weiß Heimdalls27   Horn verborgen\n"
        "\n"
        "Unter dem himmelhohen   heiligen Baum.\n"
        "Einen Strom seh ich stürzen   mit starkem Fall\n"
        "Aus Walvaters Pfand:   wißt ihr was das bedeutet?15\n"
        "\n"
        "32\n"
        "Östlich saß die Alte   im Eisengebüsch\n"
        "\n"
        "Und fütterte dort   Fenrirs Geschlecht.\n"
        "Von ihnen allen   wird eins das schlimmste:\n"
        "Des Mondes Mörder   übermenschlicher Gestalt.12\n"
        "\n"
        "33\n"
        "Ihn mästet das Mark   gefällter Männer,\n"
        "\n"
        "Der Seligen Saal   besudelt das Blut.\n"
        "Der Sonne Schein dunkelt   in kommenden Sommern,\n"
        "Alle Wetter wüthen:   wißt ihr was das bedeutet?\n"
        "\n"
        "34\n"
        "Da saß am Hügel   und schlug die Harfe\n"
        "\n"
        "Der Riesin Hüter,   der heitre Egdir.\n"
        "Vor ihm sang   im Vogelwalde\n"
        "Der hochrothe Hahn,   geheißen Fialar.\n"
        "[8]\n"
        "35\n"
        "Der schwarzrothe Hahn   in den Sälen Hels.\n"
        "\n"
        "36\n"
        "Ich sah dem Baldur,49   dem blühenden Opfer,\n"
        "\n"
        "Odhins Sohne,   Unheil drohen.\n"
        "Gewachsen war   über die Wiesen hoch\n"
        "Der zarte, zierliche   Zweig der Mistel.\n"
        "\n"
        "37\n"
        "Von der Mistel kam,   so dauchte mich\n"
        "\n"
        "Häßlicher Harm,   da Hödur schoß.\n"
        "(Baldurs Bruder   war kaum geboren,\n"
        "Als einnächtig Odhins   Erbe zum Kampf ging.30. 53\n"
        "\n"
        "Die Hände nicht wusch er,   das Haar nicht kämmt’ er,\n"
        "Eh er zum Bühle trug   Baldurs Tödter.)\n"
        "Doch Frigg beklagte   in Fensal dort\n"
        "Walhalls Verlust:   wißt ihr was das bedeutet?\n"
        "\n"
        "38\n"
        "In Ketten lag   im Quellenwalde\n"
        "\n"
        "In Unholdgestalt   der arge Loki.\n"
        "Da sitzt auch Sigyn   unsanfter Geberde,\n"
        "Des Gatten waise:   wißt ihr was das bedeutet?50\n"
        "\n"
        "39\n"
        "Gewoben weiß da   Wala Todesbande,\n"
        "\n"
        "Und fest geflochten   die Feßel aus Därmen.\n"
        "Viel weiß der Weise,   weit seh ich voraus\n"
        "Der Welt Untergang,   der Asen Fall.34\n"
        "Grässlich heult Garm[WS 2] 22   vor der Gnupahöhle,\n"
        "Die Feßel bricht   und Freki rennt.\n"
        "\n"
        "40\n"
        "Ein Strom wälzt ostwärts   durch Eiterthäler\n"
        "\n"
        "Schlamm und Schwerter,   der Slidur4 heißt.\n"
        "\n"
        "41\n"
        "Nördlich stand   an den Nidabergen\n"
        "\n"
        "Ein Saal aus Gold   für Sindris Geschlecht.\n"
        "Ein andrer stand   auf Okolnir\n"
        "Des Riesen Biersaal,   Brimir genannt.52\n"
        "[9]\n"
        "42\n"
        "Einen Saal seh ich,   der Sonne fern\n"
        "\n"
        "In Nastrand,52[WS 3] die Thüren   sind nordwärts gekehrt.\n"
        "Gifttropfen fallen   durch die Fenster nieder;\n"
        "Mit Schlangenrücken   ist der Saal gedeckt.\n"
        "\n"
        "43\n"
        "Im starrenden Strome   stehn da und waten\n"
        "\n"
        "Meuchelmörder   und Meineidige\n"
        "(Und die Andrer Liebsten   ins Ohr geraunt).\n"
        "Da saugt Nidhöggr   die entseelten Leiber,\n"
        "Der Menschenwürger:   wißt ihr was das bedeutet?\n"
        "\n"
        "44\n"
        "Viel weiß der Weise,   sieht weit voraus\n"
        "\n"
        "Der Welt Untergang,   der Asen Fall.\n"
        "\n"
        "45\n"
        "Brüder befehden sich   und fällen einander,\n"
        "\n"
        "Geschwister sieht man   die Sippe brechen.\n"
        "Der Grund erdröhnt,   üble Disen fliegen;\n"
        "Der Eine schont   des Andern nicht mehr.\n"
        "\n"
        "46\n"
        "Unerhörtes eräugnet sich,   großer Ehbruch.\n"
        "\n"
        "Beilalter, Schwertalter,   wo Schilde krachen,\n"
        "Windzeit, Wolfszeit   eh die Welt zerstürzt.\n"
        "\n"
        "47\n"
        "Mimirs Söhne spielen,   der Mittelstamm entzündet sich\n"
        "\n"
        "Beim gellenden Ruf   des Giallarhorns.\n"
        "Ins erhobne Horn   bläst Heimdall laut,\n"
        "Odhin murmelt   mit Mimirs Haupt.\n"
        "\n"
        "48\n"
        "Yggdrasil zittert,   die Esche, doch steht sie,\n"
        "\n"
        "Es rauscht der alte Baum,   da der Riese frei wird.\n"
        "(Sie bangen alle   in den Banden Hels\n"
        "Bevor sie Surturs4   Flamme verschlingt.)\n"
        "Grässlich heult Garm   vor der Gnupahöhle,\n"
        "Die Feßel bricht   und Freki rennt.\n"
        "\n"
        "49\n"
        "Hrym51 fährt von Osten   und hebt den Schild,\n"
        "\n"
        "Jörmungandr wälzt sich   im Jötunmuthe.\n"
        "Der Wurm schlägt die Flut,   der Adler facht,\n"
        "Leichen zerreißt er;   los wird Naglfar.\n"
        "[10]\n"
        "50\n"
        "Der Kiel fährt von Osten,   da kommen Muspels Söhne\n"
        "\n"
        "Über die See gesegelt;   sie steuert Loki.\n"
        "Des Unthiers Abkunft   ist all mit dem Wolf;\n"
        "Auch Bileists Bruder   ist ihm verbündet.\n"
        "\n"
        "51\n"
        "Surtur4. 51 fährt von Süden   mit flammendem Schwert,\n"
        "\n"
        "Von seiner Klinge scheint   die Sonne der Götter.\n"
        "Steinberge stürzen,   Riesinnen straucheln,\n"
        "Zu Hel fahren Helden,   der Himmel klafft.\n"
        "\n"
        "52\n"
        "Was ist mit den Asen?   was ist mit den Alfen?\n"
        "\n"
        "All Jötunheim ächzt,   die Asen versammeln sich.\n"
        "Die Zwerge stöhnen   vor steinernen Thüren,\n"
        "Der Bergwege Weiser:   wißt ihr was das bedeutet?\n"
        "\n"
        "53\n"
        "Da hebt sich Hlins35   anderer Harm,\n"
        "\n"
        "Da Odin eilt   zum Angriff des Wolfs.\n"
        "Belis Mörder35   mißt sich mit Surtur;\n"
        "Schon fällt Friggs   einzige Freude.\n"
        "\n"
        "54\n"
        "Nicht säumt Siegvaters   erhabner Sohn\n"
        "\n"
        "Mit dem Leichenwolf,   Widar, zu fechten:\n"
        "Er stößt dem Hwedrungssohn   den Stahl ins Herz\n"
        "Durch gähnenden Rachen:   so rächt er den Vater.\n"
        "\n"
        "55\n"
        "Da kommt geschritten   Hlodyns schöner Erbe,\n"
        "\n"
        "Wider den Wurm   wendet sich Odins Sohn.\n"
        "Muthig trifft ihn   Midgards Segner.\n"
        "Doch fährt neun Fuß weit   Fiörgyns Sohn\n"
        "Weg von der Natter,   die nichts erschreckte.\n"
        "Alle Wesen müßen   die Weltstatt räumen.\n"
        "\n"
        "56\n"
        "Schwarz wird die Sonne,   die Erde sinkt ins Meer,\n"
        "\n"
        "Vom Himmel schwinden   die heitern Sterne.\n"
        "Glutwirbel umwühlen   den allnährenden Weltbaum,\n"
        "Die heiße Lohe   beleckt den Himmel.\n"
        "\n"
        "57\n"
        "Da33 seh ich auftauchen   zum andernmale\n"
        "\n"
        "Aus dem Waßer die Erde   und wieder grünen.\n"
        "[11]\n"
        "\n"
        "Die Fluten fallen,   darüber fliegt der Aar,\n"
        "Der auf dem Felsen   nach Fischen weidet.\n"
        "\n"
        "58\n"
        "Die Asen einen sich   auf dem Idafelde,\n"
        "\n"
        "Über den Weltumspanner   zu sprechen, den großen.\n"
        "Uralter Sprüche   sind sie da eingedenk,\n"
        "Von Fimbultyr   gefundner Runen.\n"
        "\n"
        "59\n"
        "Da werden sich wieder   die wundersamen\n"
        "\n"
        "Goldenen Bälle   im Grase finden,\n"
        "Die in Urzeiten   die Asen hatten,\n"
        "Der Fürst der Götter   und Fiölnirs20 Geschlecht.\n"
        "\n"
        "60\n"
        "Da werden unbesät   die Äcker tragen,\n"
        "\n"
        "Alles Böse beßert sich,   Baldur kehrt wieder.\n"
        "In Heervaters Himmel   wohnen Hödur und Baldur,\n"
        "Die walweisen Götter.   Wißt ihr was das bedeutet?\n"
        "\n"
        "61\n"
        "Da kann Hönir selbst   sein Looß sich kiesen,\n"
        "\n"
        "Und beider Brüder   Söhne bebauen\n"
        "Das weite Windheim.   Wißt ihr was das bedeutet?\n"
        "\n"
        "62\n"
        "Einen Saal seh ich   heller als die Sonne,\n"
        "\n"
        "Mit Gold bedeckt   auf Gimils Höhn:3. 17. 52\n"
        "Da werden bewährte   Leute wohnen\n"
        "Und ohne Ende   der Ehren genießen.\n"
        "\n"
        "63\n"
        "Da reitet der Mächtige   zum Rath der Götter,\n"
        "\n"
        "Der Starke von Oben,   der Alles steuert.\n"
        "Den Streit entscheidet er,   schlichtet Zwiste,\n"
        "Und ordnet ewige   Satzungen an.\n"
        "\n"
        "64\n"
        "Nun kommt der dunkle   Drache geflogen,\n"
        "\n"
        "Die Natter hernieder   aus Nidafelsen.\n"
        "Das Feld überfliegend   trägt er auf den Flügeln\n"
        "Nidhöggurs Leichen — und nieder senkt er sich.";

    write_chunk_to(ogg, chunk1);
    write_chunk_to(ogg, chunk2);
    write_chunk_to(ogg, chunk3);
    write_chunk_to(ogg, chunk4);

    ogg = ov_format_close(ogg);

    unlink(fname);

    uint8_t mem[100 * (100 + 2 * 200 + 400)] = {0};

    lower_format = ov_format_from_memory(mem, sizeof(mem), OV_WRITE);
    // lower_format = ov_format_open("/tmp/oggout2.ogg", OV_WRITE);
    testrun(0 != lower_format);

    ogg = ov_format_as(lower_format, "ogg", &options, 0);

    char const *c100 =
        "1111111111111111111111111111111111111111111111111111111111111111111111"
        "111111111111111111111111111111";

    char const *c200 =
        "......................................................................"
        ".............................."
        "......................................................................"
        "..............................";

    char const *c400 =
        "4444444444444444444444444444444444444444444444444444444444444444444444"
        "444444444444444444444444444444"
        "4444444444444444444444444444444444444444444444444444444444444444444444"
        "444444444444444444444444444444"
        "4444444444444444444444444444444444444444444444444444444444444444444444"
        "444444444444444444444444444444"
        "4444444444444444444444444444444444444444444444444444444444444444444444"
        "444444444444444444444444444444";

    testrun(sizeof(mem) >=
            60 * (strlen(c100) + strlen(c200) + strlen(c400) + strlen(c200)));

    write_chunk_to(ogg, "Thats the Beginning");

    for (size_t i = 0; i < 60; i++) {

        write_chunk_to(ogg, c100);
        write_chunk_to(ogg, c200);
        write_chunk_to(ogg, c400);
        write_chunk_to(ogg, c200);
    }

    write_chunk_to(ogg, "Thats the End");

    uint8_t *end_ptr = 0;
    ov_format_attach_end_ptr_tracker(ogg, &end_ptr);

    // Check that only the first page has 'begin of stream' flag set

    // Check that all but the last page have neither 'begin' nor 'end of stream'
    // flags set

    ogg = ov_format_close(ogg);
    testrun(0 == ogg);

    testrun(check_ogg_integrity(mem, end_ptr - mem));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static uint32_t crc32_ogg_manual(uint8_t const *data, size_t len) {

    uint32_t crc = 0;
    uint32_t d = 0;

    for (size_t i = 0; i < len; ++i) {

        d = data[i];
        crc ^= d << 24;
        for (size_t k = 0; k < 8; ++k) {

            if (crc & 0x80000000) {

                crc = (crc << 1) ^ 0x104c11db7;

            } else {

                crc = crc << 1;
            }
        }
    }

    return crc;
}

/*----------------------------------------------------------------------------*/

int test_crc_ogg() {

    uint8_t const page0[] = {
        0x4f, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xf0, 0x8b, 0xe9, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x13, 0x4f, 0x70, 0x75, 0x73, 0x48, 0x65, 0x61, 0x64,
        0x01, 0x02, 0x38, 0x01, 0x80, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t *page0_copy = calloc(1, sizeof(page0));

    memcpy(page0_copy, &page0, sizeof(page0));

    // get original checksum

    uint32_t original_checksum = page0[OFFSET_CRC32 + 0];
    original_checksum <<= 8;
    original_checksum += page0[OFFSET_CRC32 + 1];
    original_checksum <<= 8;
    original_checksum += page0[OFFSET_CRC32 + 2];
    original_checksum <<= 8;
    original_checksum += page0[OFFSET_CRC32 + 3];

    uint32_t correct_checksum = 0x1082e2fb;

    fprintf(stderr,
            "Original checksum: %" PRIu32 "   Manual: %" PRIu32
            "   Calculated checksum: %" PRIu32
            "   Manually calculated checksum: %" PRIu32 "\n",
            original_checksum,
            correct_checksum,
            ov_crc32_ogg(0, page0_copy, sizeof(page0)),
            crc32_ogg_manual(page0, sizeof(page0)));

    testrun(correct_checksum == crc32_ogg_manual(page0, sizeof(page0)));
    testrun(correct_checksum == ov_crc32_ogg(0, page0_copy, sizeof(page0)));

    ov_free(page0_copy);

    // Now check whether the checksum in the ogg format changes if the
    // payload changes

    ov_format_ogg_install(0);

    ov_format_ogg_options options = {

        .chunk_length_ms = 20,
        .samplerate_hz = 48000,

    };

    uint8_t mem[1000] = {0};

    ov_format *memformat = ov_format_from_memory(mem, sizeof(mem), OV_WRITE);
    testrun(0 != memformat);

    ov_format *ogg = ov_format_as(memformat, "ogg", &options, 0);
    testrun(0 != ogg);

    write_chunk_to(ogg, "Eins zwei drei vier...\n");

    ov_format_close(ogg);

    uint32_t crc1 = decode_32(mem + 22);

    memset(mem, 0, sizeof(mem));

    memformat = ov_format_from_memory(mem, sizeof(mem), OV_WRITE);
    testrun(0 != memformat);

    ogg = ov_format_as(memformat, "ogg", &options, 0);
    testrun(0 != ogg);

    write_chunk_to(ogg, "Eins zwoh drei vier...\n");

    ov_format_close(ogg);

    uint32_t crc2 = decode_32(mem + 22);

    // Now we got 2 oggs in mem1 and mem2 with slightly different payload.
    // The CRCs should not equal...

    testrun(crc1 != crc2);

    // The first page should contain the 'begin of stream' flag
    testrun(0x02 == (0x02 & mem[5]));

    // Check that the header is taken into account when calculating the
    // CRC checksum.
    // We will create the same ogg chunk again, with the exception of
    // the stream serial changed.
    memset(mem, 0, sizeof(mem));

    memformat = ov_format_from_memory(mem, sizeof(mem), OV_WRITE);
    testrun(0 != memformat);

    options.stream_serial = 12;

    ogg = ov_format_as(memformat, "ogg", &options, 0);
    testrun(0 != ogg);

    write_chunk_to(ogg, "Eins zwei drei vier...\n");

    ov_format_close(ogg);

    crc2 = decode_32(mem + 22);

    // Now we got 2 oggs in mem1 and mem2 with slightly different payload.
    // The CRCs should not equal...
    testrun(crc1 != crc2);

    // The first page should contain the 'begin of stream' flag
    testrun(0x02 == (0x02 & mem[5]));

    // The last page should contain the 'end of stream' flag

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool next_chunk_is(ov_format *format, char const *ref) {

    ov_buffer *chunkbuf = ov_format_payload_read_chunk(format, 1);

    if (0 == chunkbuf) {

        testrun_log_error("Could not read another chunk from Ogg");
        return false;

    } else {

        char const *read_str = (char const *)ov_buffer_data(chunkbuf);
        fprintf(stderr,
                "Expect '%s' - got '%s'\n",
                ov_string_sanitize(ref),
                ov_string_sanitize(read_str));
        bool ok = (0 == strcmp(ref, read_str));
        chunkbuf = ov_buffer_free(chunkbuf);
        return ok;
    }
}

/*----------------------------------------------------------------------------*/

int test_impl_next_chunk() {

    char const *chunk1 = "Abra cadabra";
    char const *chunk2 = "alpha beta gamma delta";
    char const *chunk3 = "Karacho, mit. Was soll das bedeuten?";

    char const *chunk_256 =
        "123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
        "123456789 1234567890 1234567890"
        "123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
        "123456789 1234567890 1234567890"
        "123456789 123456789 123456789 123456789 123456789 fuenfS";

    ov_format_ogg_options opts = {
        .chunk_length_ms = 20,
        .samplerate_hz = 48000,
    };

    char const *fname = "/tmp/test_impl_next_chunk.ogg";
    ov_format *ogg =
        ov_format_as(ov_format_open(fname, OV_WRITE), "ogg", &opts, 0);

    fprintf(stderr, "Using %s as temporary file\n", fname);

    write_chunk_to(ogg, chunk1);
    write_chunk_to(ogg, chunk2);
    write_chunk_to(ogg, chunk3);
    write_chunk_to(ogg, chunk1);
    write_chunk_to(ogg, chunk2);
    write_chunk_to(ogg, chunk1);
    write_chunk_to(ogg, chunk3);
    write_chunk_to(ogg, chunk_256);

    ogg = ov_format_close(ogg);
    testrun(0 == ogg);

    // Now try to read back all the chunks in correct order...

    ogg = ov_format_as(ov_format_open(fname, OV_READ), "ogg", &opts, 0);

    testrun(next_chunk_is(ogg, chunk1));
    testrun(next_chunk_is(ogg, chunk2));
    testrun(next_chunk_is(ogg, chunk3));
    testrun(next_chunk_is(ogg, chunk1));
    testrun(next_chunk_is(ogg, chunk2));
    testrun(next_chunk_is(ogg, chunk1));
    testrun(next_chunk_is(ogg, chunk3));
    testrun(next_chunk_is(ogg, chunk_256));

    ogg = ov_format_close(ogg);
    unlink(fname);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ogg_new_stream() {

    ov_format *ogg_data = ov_format_from_memory(0, 64, OV_WRITE);
    testrun(0 != ogg_data);

    ov_format *ogg = ov_format_as(ogg_data, "ogg", 0, 0);
    testrun(0 != ogg);

    testrun(!ov_format_ogg_new_stream(0, 0));
    testrun(ov_format_ogg_new_stream(ogg, 0));
    testrun(ov_format_ogg_new_stream(ogg, 20));

    ogg = ov_format_close(ogg);
    testrun(0 == ogg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ogg_select_stream() {

    testrun(!ov_format_ogg_select_stream(0, 0));
    testrun(!ov_format_ogg_select_stream(0, 12));

    char const *chunk1 = "Abra cadabra";
    char const *chunk2 = "alpha beta gamma delta";
    char const *chunk3 = "Karacho, mit. Was soll das bedeuten?";

    char const *chunk_256 =
        "123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
        "123456789 1234567890 1234567890"
        "123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
        "123456789 1234567890 1234567890"
        "123456789 123456789 123456789 123456789 123456789 fuenfS";

    // Create a file with 2 distinct streams

    char const *fname = "/tmp/test_ov_format_ogg_select_stream.ogg";
    // Stream serial should be set to 0
    ov_format *ogg = ov_format_as(ov_format_open(fname, OV_WRITE), "ogg", 0, 0);
    testrun(0 != ogg);

    write_chunk_to(ogg, chunk1);
    write_chunk_to(ogg, chunk2);
    write_chunk_to(ogg, chunk1);
    write_chunk_to(ogg, chunk2);
    write_chunk_to(ogg, chunk1);

    testrun(ov_format_ogg_new_stream(ogg, 132));

    write_chunk_to(ogg, chunk3);
    write_chunk_to(ogg, chunk2);
    write_chunk_to(ogg, chunk3);
    write_chunk_to(ogg, chunk2);

    // Force new page for same stream...
    for (size_t i = 0; i < 0x100; ++i) {
        write_chunk_to(ogg, chunk_256);
    }

    testrun(ov_format_ogg_new_stream(ogg, 256));

    write_chunk_to(ogg, chunk_256);

    ogg = ov_format_close(ogg);
    testrun(0 == ogg);

    // Read back
    ogg = ov_format_as(ov_format_open(fname, OV_READ), "ogg", 0, 0);
    testrun(0 != ogg);

    // Try to read back stream 0

    testrun(next_chunk_is(ogg, chunk1));
    testrun(next_chunk_is(ogg, chunk2));
    testrun(next_chunk_is(ogg, chunk1));
    testrun(next_chunk_is(ogg, chunk2));
    testrun(next_chunk_is(ogg, chunk1));

    // No more
    testrun(0 == ov_format_payload_read_chunk(ogg, 1));

    ogg = ov_format_close(ogg);
    testrun(0 == ogg);

    // Try to read back stream 132

    ogg = ov_format_as(ov_format_open(fname, OV_READ), "ogg", 0, 0);
    testrun(0 != ogg);

    testrun(ov_format_ogg_select_stream(ogg, 132));

    testrun(next_chunk_is(ogg, chunk3));
    testrun(next_chunk_is(ogg, chunk2));
    testrun(next_chunk_is(ogg, chunk3));
    testrun(next_chunk_is(ogg, chunk2));

    for (size_t i = 0; i < 0x100; ++i) {
        if (!next_chunk_is(ogg, chunk_256)) {
            fprintf(stderr, "failed at %zu\n", i);
            testrun(0);
        }
    }

    testrun(0 == ov_format_payload_read_chunk(ogg, 1));

    ogg = ov_format_close(ogg);
    testrun(0 == ogg);

    // Try to read back stream 256
    // Use options to select our stream

    ov_format_ogg_options opts = {
        .stream_serial = 256,
    };

    ogg = ov_format_as(ov_format_open(fname, OV_READ), "ogg", &opts, 0);
    testrun(0 != ogg);

    testrun(next_chunk_is(ogg, chunk_256));

    testrun(0 == ov_format_payload_read_chunk(ogg, 1));

    ogg = ov_format_close(ogg);
    testrun(0 == ogg);

    unlink(fname);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_format_registry_clear(0);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_ogg",
            test_ov_format_ogg_install,
            test_impl_write_chunk,
            test_crc_ogg,
            test_impl_next_chunk,
            test_ov_format_ogg_new_stream,
            test_ov_format_ogg_select_stream,
            tear_down);

/*----------------------------------------------------------------------------*/
