#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gmime/gmime.h>
#include <sys/socket.h>
#include <arpa/inet.h>
gchar* to;
gchar* cc;
gchar* bcc;
gchar* subject;
gchar* body;
gchar* from;
gchar* server;

GOptionEntry entries[] =
  {
   {"to", 't', 0, G_OPTION_ARG_STRING, &to, "To address", "Address"},
   {"cc", 'c', 0, G_OPTION_ARG_STRING, &cc, "Cc address", "Address"},
   {"bcc", 'b', 0, G_OPTION_ARG_STRING, &bcc, "Bcc address", "Address"},
   {"subject", 's', 0, G_OPTION_ARG_STRING, &subject, "Subject", "Subject"},
   {"body", 'B', 0, G_OPTION_ARG_STRING, &body, "Body", "Text"},
   {"from", 'f', 0, G_OPTION_ARG_STRING, &from, "From address", "Address"},
   {"server", 'S', 0, G_OPTION_ARG_STRING, &server, "Server", "Server"},
   { NULL }
  };

int waitForReply(GIOChannel* channel);
int sendMessage(char* server, GMimeMessage* message);

static void
write_message_to_file (GMimeMessage *message, int fd)
{
	GMimeStream *stream;
	
	/* create a new stream for writing to stdout */
	stream = g_mime_stream_fs_new (fd);
	g_mime_stream_fs_set_owner ((GMimeStreamFs *) stream, FALSE);
	
	/* write the message to the stream */
	g_mime_object_write_to_stream ((GMimeObject *) message, stream);
	
	/* flush the stream (kinda like fflush() in libc's stdio) */
	g_mime_stream_flush (stream);
	
	/* free the output stream */
	g_object_unref (stream);
}

int main(int argc, char** argv){
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new("- send an email message");
  g_option_context_add_main_entries(context, entries, NULL);
  if(!g_option_context_parse(context, &argc, &argv, &error)){
    g_print("option parsing failed: %s\n", error->message);
    exit(1);
  }
  GMimeMessage* message = g_mime_message_new(FALSE);
  if(subject != NULL)
    g_mime_message_set_subject(message, subject);
  else
    g_mime_message_set_subject(message, "(no subject)");
  if(to == NULL){
    g_print("To: cannot be empty\n");
    exit(1);
  }
  if(from == NULL){
    g_print("From: cannot be empty\n");
    exit(1);
  }
  if(body == NULL){
    g_print("Body cannot be empty\n");
    exit(1);
  }
  if(server == NULL){
    server = "127.0.0.1";
  }
  g_mime_message_add_recipient(message, GMIME_RECIPIENT_TYPE_TO, NULL, to);
  if(cc != NULL){
    g_mime_message_add_recipient(message, GMIME_RECIPIENT_TYPE_CC, NULL, cc);
  }
  if(bcc != NULL){
    g_mime_message_add_recipient(message, GMIME_RECIPIENT_TYPE_BCC, NULL, bcc);
  }
  GMimePart* messageBody = g_mime_part_new_with_type("text", "plain");
  if (messageBody == NULL)
    g_error("Could not create message body");
  GMimeStream* stream = g_mime_stream_mem_new_with_buffer(body, strlen(body));
  if (stream == NULL)
    g_error("Could not create stream"); 
  GMimeDataWrapper* content = g_mime_data_wrapper_new_with_stream(stream, GMIME_CONTENT_ENCODING_DEFAULT);
  if (content == NULL)
    g_error("Could not create data wrapper");
  g_object_unref(stream);
  g_mime_part_set_content_object(messageBody, content);
  g_object_unref(content);
  g_mime_message_set_mime_part(message, GMIME_OBJECT(messageBody));
  g_mime_message_set_date(message, time(0), 0);
  write_message_to_file(message, fileno(stdout));
  g_printf("\n");
  if (sendMessage(server, message) < 0)
    g_printf("Send message failed\n");
  else
    g_printf("Success!");
  g_object_unref(message);
  return 0;
}

int sendMessage(char* server, GMimeMessage* message){
  int smtpSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(smtpSocket < 0){
    g_printf("Socket Error\n");
    return -1;
  }
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(server);
  addr.sin_port = htons(587);
  int result = connect(smtpSocket, (struct sockaddr*)&addr, sizeof(addr));
  if(result < 0){
    close(smtpSocket);
    g_printf("Connection Failed\n");
    return -1;
  }
  GIOChannel* channel = g_io_channel_unix_new(smtpSocket);
  int status;
  gchar* cmd;
  gsize written;
  GError *error;
  status = waitForReply(channel);
  if(status != 220){
    return -1;
  }
  //Send Hello
  cmd = g_strdup_printf("HELO %s\r\n", server);
  g_io_channel_write_chars(channel, cmd, strlen(cmd), &written, &error);
  g_io_channel_flush(channel, NULL);
  g_free(cmd);
  status = waitForReply(channel);
  if(status != 250){
    return -1;
  }
  //Send From
  cmd = g_strdup_printf("MAIL FROM:<%s>\r\n", from);
  g_io_channel_write_chars(channel, cmd, strlen(cmd), &written, &error);
  g_io_channel_flush(channel, NULL);
  g_free(cmd);
  status = waitForReply(channel);
  if(status != 250){
    return -1;
  }
  //Send To
  cmd = g_strdup_printf("RCPT TO:<%s>\r\n", to);
  g_io_channel_write_chars(channel, cmd, strlen(cmd), &written, &error);
  g_io_channel_flush(channel, NULL);
  g_free(cmd);
  status = waitForReply(channel);
  if(status != 250){
    return -1;
  }
  //Send CC
  if(cc != NULL){
    cmd = g_strdup_printf("RCPT CC:<%s>\r\n", cc);
    g_io_channel_write_chars(channel, cmd, strlen(cmd), &written, &error);
    g_io_channel_flush(channel, NULL);
    g_free(cmd);
    status = waitForReply(channel);
    if(status != 250){
      return -1;
    }
  }
  //Send Bcc
  if(bcc != NULL){
    cmd = g_strdup_printf("RCPT BCC:<%s>\r\n", bcc);
    g_io_channel_write_chars(channel, cmd, strlen(cmd), &written, &error);
    g_io_channel_flush(channel, NULL);
    g_free(cmd);
    status = waitForReply(channel);
    if(status != 250){
      return -1;
    }
  }
  //Send Data
  cmd = g_strdup_printf("DATA\r\n");
  g_io_channel_write_chars(channel, cmd, strlen(cmd), &written, &error);
  g_io_channel_flush(channel, NULL);
  g_free(cmd);
  status = waitForReply(channel);
  if(status != 354){
    return -1;
  }
  write_message_to_file(message, smtpSocket);
  cmd = "\r\n.\r\n";
  g_io_channel_write_chars(channel, cmd, strlen(cmd), &written, &error);
  g_io_channel_flush(channel, NULL);
  status = waitForReply(channel);
  if(status != 250){
    return -1;
  }
  cmd = "QUIT\r\n";
  g_io_channel_write_chars(channel, cmd, strlen(cmd), &written, &error);
  g_io_channel_flush(channel, NULL);
  status = waitForReply(channel);
  if(status != 221){
    return -1;
  }
  
}

int waitForReply(GIOChannel* channel){
  gchar* reply;
  gsize length;
  GError* error = NULL;
  GIOStatus readStatus;
  int status = -1;
  readStatus = g_io_channel_read_line(channel, &reply, &length, NULL, &error);
  //g_printf("read: %d %d\n", readStatus, length);
  if(readStatus == G_IO_STATUS_NORMAL){
    sscanf(reply, "%d", &status);
    //g_printf("SMTP status: %d\n", status);
    g_free(reply);
  }
  return status;
}
