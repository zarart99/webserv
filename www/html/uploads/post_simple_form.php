<!DOCTYPE html>
<html>
<head><title>Formulaire POST simple</title></head>
<body>
  <h2>Send your message</h2>
  <form action="http://localhost:8001/uploads/post_simple.php" method="POST">
	<textarea name="message" rows="5" cols="40" placeholder="Type your message here"></textarea><br><br>
	<button type="submit">Envoyer</button>
  </form>
</body>
</html>