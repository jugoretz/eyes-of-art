#pragma once
struct Artwork {
  const char *filename;
  const char *artist;
  const char *title;
  const char *date;
};

Artwork playlist[] = {
  { "ringgold.gif", "Faith Ringgold", "Dancing at the Louvre", "1991" },
  { "renoir.gif", "Pierre-Auguste Renoir", "La Loge", "1874" },
  { "rembrandt.gif", "Rembrandt van Rijn", "Self-Portrait", "1659" },
  { "picasso.gif", "Pablo Picasso", "Portrait of Gertrude Stein", "1905-06" },
  { "jarvis.gif", "John Wesley Jarvis", "Black Hawk and His Son Whirling Thunder", "1833" },
  { "ingres.gif", "Jean-Auguste-Dominique Ingres", "Napoleon on His Imperial Throne", "1806" },
  { "giorgione.gif", "Giorgione", "La vecchia", "c. 1502–08," },
  { "micromosaic.gif", "Unrecorded Artist", "Micromosaic Icon with the Virgin Hodegetria Dexiokratousa", "First Quarter 13th Century" },
  { "memling_christ.gif", "Hans Memling", "Christ Blessing", "1481" },
  { "almedina.gif", "Fernando Yáñez de la Almedina", "Head of Christ", "c. 1506" },
  { "vlad.gif", "Unrecorded Artist", "Portrait of Vlad III Dracula (Ambras Portrait)", "Second Half 16th Century" },
  { "vanderweyden_2.gif", "Rogier van der Weyden", "The Seven Sacraments", "1440-45" },
  { "piero_2.gif", "Piero della Francesca", "Brera Madonna (Montefeltro Altarpiece)", "1472-74" },
  { "antikythera.gif", "Unrecorded Artist", "Antikythera Youth", "340-330 B.C.E." },
  { "hequet_ingres.gif", "Laurence-Augustine Jubé Héquet (Madame Héquet)", "Ingres as a Young Man", "ca. 1850–60" },
  { "vaneyck_canon.gif", "Jan van Eyck", "The Virgin and Child with Canon Joris van der Paele", "1436" },
  { "piero_1.gif", "Piero della Francesca", "Brera Madonna (Montefeltro Altarpiece)", "1472-74" },
  { "rolanda.gif", "Rosa Rolanda", "Self-Portrait", "1939" },
  { "petrus_2.gif", "Petrus Christus", "Portrait of a Carthusian", "1446" },
  { "belmonte.gif", "Master of Belmonte", "St Michael Defeating the Devil", "1450-1500" },
  { "durer.gif", "Albrecht Dürer", "Self-Portrait with Fur-Trimmed Robe", "1500" },
  { "mantegna.gif", "Andrea Mantegna", "Camera picta (Camera degli Sposi)", "1465-74" },
  { "vanderweyden_4.gif", "Rogier van der Weyden", "The Seven Sacraments", "1440-45" },
  { "brown_1.gif", "Ford Madox Brown", "The Last of England", "1852-55" },
  { "vanderweyden_1.gif", "Rogier van der Weyden", "The Seven Sacraments", "1440-45" },
  { "benoist.gif", "Marie Guillelmine Benoist", "Madame Philippe Panon Desbassayns de Richemont and Her Son, Eugène", "1802" },
  { "stuart.gif", "Gilbert Stuart", "Catherine Brass Yates (Mrs. Richard Yates)", "1793-94" },
  { "brown_2.gif", "Ford Madox Brown", "The Last of England", "1852-55" },
  { "sanvitale.gif", "Unrecorded Artist", "San Vitale", "546-56" },
  { "vanderweyden_5.gif", "Rogier van der Weyden", "The Seven Sacraments", "1440-45" },
  { "memling.gif", "Hans Memling", "Triptych with John the Baptist and John the Evangelist, with the mystic marriage of Saint Catherine", "1479" },
  { "vanderweyden_3.gif", "Rogier van der Weyden", "The Seven Sacraments", "1440-45" },
  { "fouquet.gif", "Jean Fouquet", "Virgin surrounded by seraphim and cherubim, Melun Diptych", "c. 1455" },
  { "arnolfini.gif", "Jan van Eyck", "Portrait of Giovanni(?) Arnolfini and his Wife", "1434" },
  { "petrus_1.gif", "Petrus Christus", "Portrait of a Carthusian", "1446" },
  { "vaneyck_man.gif", "Jan van Eyck", "Portrait of a Man (Self Portrait?)", "1433" },
  { "millais.gif", "John Everett Millais", "Spring (Apple Blossoms)", "1859" }
};

const int playlistCount = sizeof(playlist) / sizeof(playlist[0]);
