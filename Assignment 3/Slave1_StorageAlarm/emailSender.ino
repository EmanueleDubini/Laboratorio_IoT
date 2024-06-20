void emailSender() {
  EMailSender::EMailMessage message;

  // Oggetto del messaggio
  message.subject = "Allarme rilevato";

  // Inizio del contenuto HTML
  String htmlContent = "<html><body>";  // Rimuovi l'attributo text-align dal body

  // Aggiungi un contenitore centrale
  htmlContent += "<div style='text-align: center;'>";

  // Aggiungi un titolo
  htmlContent += "<h1 style='color: red;'>Allarme rilevato</h1>";

  if (flameValue == HIGH && motionValue == LOW) {
    // Aggiungi un'immagine
    htmlContent += "<img src='https://c8.alamy.com/compit/2bw3x3d/vettore-cartoon-figura-disegno-disegno-illustrazione-concettuale-di-uomo-arrabbiato-o-uomo-d-affari-che-porta-torcia-fiammeggiante-e-jerry-puo-o-jerrycan-con-gas-o-benzina-2bw3x3d.jpg' alt='Immagine non trovata' style='width: 40%; max-width: 600px;'>";  // Correggi l'URL con quello della tua immagine
    htmlContent += "<p style='font-size: 16px;'><strong><br>Movimento inaspettato rilevato in magazzino.<br> Fiamme rilevate in magazzino.</strong></p>";
  } else if (flameValue == HIGH) {
    // Aggiungi un'immagine
    htmlContent += "<img src='https://cdn.dribbble.com/users/76939/screenshots/758710/flame-icon.png' alt='Immagine non trovata' style='width: 40%; max-width: 600px;'>";  // Cambia l'URL con quello della tua immagine

    htmlContent += "<p style='font-size: 16px;'><strong>Fiamme rilevate in magazzino.</strong></p>";
  } else {
    // Aggiungi un'immagine
    htmlContent += "<img src='https://cdn-icons-png.flaticon.com/512/9400/9400656.png' alt='Immagine non trovata' style='width: 40%; max-width: 600px;'>";  // Cambia l'URL con quello della tua immagine

    htmlContent += "<p style='font-size: 16px;'>Movimento inaspettato rilevato in magazzino.</p>";
  }

  // Chiudi il contenitore centrale
  htmlContent += "</div>";

  // Chiusura del contenuto HTML
  htmlContent += "</body></html>";

  // Imposta il contenuto del messaggio in HTML
  message.message = htmlContent;

  // Invia il messaggio
  if (userEmail != "Nessuna email inserita"){
  EMailSender::Response resp = emailSend.send(userEmail, message);

  // Stampa lo stato dell'invio
  Serial.println("Sending status: ");
  Serial.println(resp.status);
  Serial.println(resp.code);
  Serial.println(resp.desc);
  }
}
