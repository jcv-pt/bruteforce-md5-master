create database md5DB;

use md5DB;

CREATE TABLE sequences (
`id` int NOT NULL AUTO_INCREMENT PRIMARY KEY,
`ip` varchar(100),
`seq` int not null,
`completed` int not null
);
