/*
 * ****************************************************************************
 *  University of Milano-Bicocca
 *  DISCo - Department of Informatics, Systems and Communication
 *  Viale Sarca 336, 20126 Milano, Italy
 *
 *  Copyright © 2019-2024 by:
 *    Davide Marelli   - davide.marelli@unimib.it
 *    Paolo Napoletano - paolo.napoletano@unimib.it
 * ****************************************************************************
 *
 * MySQL server setup:
 *   - install MySQL   https://dev.mysql.com/downloads/mysql/
 *   - install and start MySQL Workbench   https://dev.mysql.com/downloads/workbench/
 *   - create a connection to the MySQL server and connect to it
 *   - Server -> Users and Privileges
 *      - Add Account
 *         - Login Name -> <same as MYSQL_USER in secrets.h>
 *         - Password -> <same as MYSQL_PASS in secrets.h>
 *         - Apply
 *   - Server -> Data import
 *      - Import from self contained file -> <select the logging.sql file>
 *      - Default Target Schema -> <your db name>
 *      - Start import
 *   - Once the ESP has stored some entries check the table records
 *      - select (double click) the 'logging' db from the 'schema' tab in the 'navigator' panel
 *      - expand 'Tables' and click the rightmost icon that appears when hovering the 'wifi_data_simple' table
 *
 */

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";

--
-- Database: `<Name><Surname>`
--
-- CREATE DATABASE IF NOT EXISTS `<Name><Surname>` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
-- USE `<Name><Surname>`;

-- --------------------------------------------------------

--
-- Struttura della tabella `wifi_data_simple`
--

DROP TABLE IF EXISTS `assignment_1`;
CREATE TABLE `assignment_1` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `datetime` timestamp NULL DEFAULT current_timestamp(),
  `ssid` varchar(45) DEFAULT NULL,
  `rssi` int(11) DEFAULT NULL,
  `tilt` boolean,
  `humidity` float,
  `temperature` float,
  `light` int unsigned,
  `alarm` boolean,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;


--
-- Indici per le tabelle `wifi_data_simple`
--

--
-- AUTO_INCREMENT per la tabella `wifi_data_simple`
--
ALTER TABLE `assignment_1`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
COMMIT;
