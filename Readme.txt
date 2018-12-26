A Simple command line SMTP client.

Sends message to SMTP server.
Runs on Cygwin.  Uses Glib and Gmime libraries.  

Command line parameters:
-to: Recipient (Required)
-cc: Cc Recipient
-bcc: Bcc Recipient
-subject: Subject (Defaults to (No Subject))
-body: Message Body (Required)
-from: Sender Address (Required)
-server: SMTP Server Address (Defaults to localhost)

The program prints out all of the message contents (recipients, date,
subject, body) and whether it was a success or failure.  It does not
use password authentication.  This program can be tested using any
server listening on port 587.  If the message was successfully sent,
"Success!" should appear.  The Glib and Gmime libraries must be
installed.

To compile:
1. Create a build directory
2. Run cmake with path to the source directory
3. Run make
