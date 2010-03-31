import java.io.*;
import java.net.*;

class Httpsqs_client {
    int i = 0;
    String SERVER, PORT, CHARSET;

    public Httpsqs_client(String server, String port, String charset) {
        SERVER = server;
        PORT = port;
        CHARSET = charset;
    }

    public String put(String queue_name, String data)
    {
    	String urlstr = "http://" + SERVER + ":" + PORT + "/?name=" + queue_name + "&opt=put";
    	URL url = null;
		try {
			url = new URL(urlstr);
		} catch (MalformedURLException e) {
			System.out.println("The httpsqs server must be error");
			return null;
		}
    	
    	URLConnection conn = null;
    	
		try {
			conn = url.openConnection();
		} catch (IOException e) {
			System.out.println("Connection error");
			return null;
		}
        conn.setDoOutput(true);
        OutputStreamWriter out = null;
		try {
			out = new OutputStreamWriter(conn.getOutputStream());
		} catch (IOException e) {
			System.out.println("Connection error");
			return null;
		}
        try {
			out.write(data);
		} catch (IOException e) {
			System.out.println("Post data error");
			return null;
		}
        try {
			out.flush();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return null;
		}
        try {
			out.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return null;
		}

        BufferedReader reader = null;
		try {
			reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return null;
		}
        String line=null;

        try {
			while((line=reader.readLine())!=null)
			{
				return line;
			}
		} catch (IOException e) {
			System.out.println("Get return data error");
			return null;
		}
        return null;
    }
}