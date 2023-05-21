import java.io.*;
import java.net.*;
import java.util.regex.*;

public class BookClient {
    public static void main(String[] args) throws IOException {
        String hostAddress = "localhost";
        int tcpPort = 7000;
        int udpPort = 8000;
        int clientId;

        if (args.length != 2) {
            System.out.println("ERROR: Provide 2 arguments: command-file, clientId");
            System.out.println("\t(1) command-file: file with commands to the server");
            System.out.println("\t(2) clientId: an integer between 1..9");
            System.exit(-1);
        }

        String commandFile = args[0];
        clientId = Integer.parseInt(args[1]);

        Socket socket = new Socket(hostAddress, tcpPort);
        BufferedReader inFromServer = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        DataOutputStream outToServer = new DataOutputStream(socket.getOutputStream());

        // Create a FileWriter object to write server responses to a file
        FileWriter fileWriter = new FileWriter("out_ClientID.txt");
        String line;
        BufferedReader fileReader = new BufferedReader(new FileReader(commandFile));
        while ((line = fileReader.readLine()) != null) {
            outToServer.writeBytes(line + "\n");
            String confirmation = inFromServer.readLine();
            if(line.charAt(0) == 'g'){
                confirmation = parser(confirmation);
                confirmation = confirmation.trim();
            }
            System.out.println(confirmation);

            // Write the server response to the file
            fileWriter.write(confirmation + "\n");
        }
        fileReader.close();
        socket.close();

        // Close the FileWriter object after all server responses have been written to the file
        fileWriter.close();
    }

    public static String parser(String input) {
        // Match each book title and its number using a regular expression
        Pattern pattern = Pattern.compile("\"([^\"]*)\"\\s+(\\d+)");
        Matcher matcher = pattern.matcher(input);

        // Format each matched title and number into a new string with titles on new lines
        StringBuilder outputBuilder = new StringBuilder();
        while (matcher.find()) {
            String title = matcher.group(1);
            String number = matcher.group(2);
            outputBuilder.append("\"").append(title).append("\" ").append(number).append("\n");
        }

        return outputBuilder.toString();
    }
}
