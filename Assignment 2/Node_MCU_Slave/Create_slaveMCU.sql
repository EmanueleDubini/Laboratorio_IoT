
-- Esegui la query per creare la tabella
DROP TABLE IF EXISTS `A2_slaveMCU`;

CREATE TABLE `A2_slaveMCU` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `datetime` timestamp NULL DEFAULT current_timestamp(),
  `ssid` varchar(45) DEFAULT NULL,
  `rssi` int(11) DEFAULT NULL,
  `ir_obstacle` boolean DEFAULT NULL,
  `flame` boolean DEFAULT NULL,
  `alarm` boolean DEFAULT NULL,
  `message` varchar(256),
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;