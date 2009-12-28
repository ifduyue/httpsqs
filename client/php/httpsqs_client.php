<?php
/*
----------------------------------------------------------------------------------------------------------------
HTTP Simple Queue Service - httpsqs client class for PHP v1.1

Author: Zhang Yan (http://blog.s135.com), E-mail: net@s135.com
This is free software, and you are welcome to modify and redistribute it under the New BSD License
----------------------------------------------------------------------------------------------------------------
Useage:
<?php
......
include_once("httpsqs_client.php");
$httpsqs = new httpsqs;

//http connect without Keep-Alive
$result = $httpsqs->put($host, $port, $charset, $name, $data); //1. PUT text message into a queue. If PUT successful, return boolean: true. If an error occurs, return boolean: false
$result = $httpsqs->get($host, $port, $charset, $name); //2. GET text message from a queue. Return the queue contents. If there is no unread queue message, return text: HTTPSQS_GET_END
$result = $httpsqs->status($host, $port, $charset, $name); //3. View queue status
$result = $httpsqs->view($host, $port, $charset, $name, $pos); //4. View the contents of the specified queue pos (id). Return the contents of the specified queue pos.
$result = $httpsqs->reset($host, $port, $charset, $name); //5. Reset the queue. If reset successful, return boolean: true. If an error occurs, return boolean: false
$result = $httpsqs->maxqueue($host, $port, $charset, $name, $num); //6. Change the maximum queue length of per-queue. If change the maximum queue length successful, return boolean: true. If  it be cancelled, return boolean: false

//http pconnect with Keep-Alive (Very fast in PHP FastCGI mode & Command line mode)
$result = $httpsqs->pput($host, $port, $charset, $name, $data); //1. PUT text message into a queue. If PUT successful, return boolean: true. If an error occurs, return boolean: false
$result = $httpsqs->pget($host, $port, $charset, $name); //2. GET text message from a queue. Return the queue contents. If there is no unread queue message, return text: HTTPSQS_GET_END
$result = $httpsqs->pstatus($host, $port, $charset, $name); //3. View queue status
$result = $httpsqs->pview($host, $port, $charset, $name, $pos); //4. View the contents of the specified queue pos (id). Return the contents of the specified queue pos.
$result = $httpsqs->preset($host, $port, $charset, $name); //5. Reset the queue. If reset successful, return boolean: true. If an error occurs, return boolean: false
$result = $httpsqs->pmaxqueue($host, $port, $charset, $name, $num); //6. Change the maximum queue length of per-queue. If change the maximum queue length successful, return boolean: true. If  it be cancelled, return boolean: false
?>
----------------------------------------------------------------------------------------------------------------
Example:
<?php
include_once("httpsqs_client.php");
$httpsqs = new httpsqs;
echo "<pre>";
$result = $httpsqs->put("127.0.0.1", 1218, "utf-8", "your_queue_name1", urlencode("text_message"));
echo "###1.put result:\r\n".$result."\r\n\r\n";

$result = $httpsqs->get("127.0.0.1", 1218, "utf-8", "your_queue_name1");
echo "###2.get result:\r\n".$result."\r\n\r\n";

$result = $httpsqs->pput("127.0.0.1", 1218, "gb2312", "your_queue_name2", urlencode("text_message"));
echo "###3.pput result:\r\n".$result."\r\n\r\n";

$result = $httpsqs->pget("127.0.0.1", 1218, "gb2312", "your_queue_name2");
echo "###4.pget result:\r\n".$result."\r\n\r\n";

$result = $httpsqs->status("127.0.0.1", 1218, "utf-8", "your_queue_name1");
echo "###5.status result:\r\n".$result."\r\n\r\n";

$result = $httpsqs->view("127.0.0.1", 1218, "utf-8", "your_queue_name1", 1);
echo "###6.view result:\r\n".$result."\r\n\r\n";

$result = $httpsqs->reset("127.0.0.1", 1218, "utf-8", "your_queue_name1");
echo "###7.reset result:\r\n".$result."\r\n\r\n";

$result = $httpsqs->maxqueue("127.0.0.1", 1218, "utf-8", "your_queue_name1", 5000000);
echo "###8.maxqueue result:\r\n".$result."\r\n\r\n";
echo "</pre>";
?>
----------------------------------------------------------------------------------------------------------------
*/

class httpsqs
{
    function http_get($host, $port, $query)
    {
        $fp = fsockopen($host, $port, $errno, $errstr, 1);
        if (!$fp)
        {
            return false;
        }
        $out = "GET ${query} HTTP/1.1\r\n";
        $out .= "Host: ${host}\r\n";
        $out .= "Connection: close\r\n";
        $out .= "\r\n";
        fwrite($fp, $out);
        $line = trim(fgets($fp));
        $header .= $line;
        list($proto, $rcode, $result) = explode(" ", $line);
        $len = -1;
        while (($line = trim(fgets($fp))) != "")
        {
            $header .= $line;
            if (strstr($line, "Content-Length:"))
            {
                list($cl, $len) = explode(" ", $line);
            }
            if (strstr($line, "Connection: close"))
            {
                $close = true;
            }
        }
        if ($len < 0)
        {
            return false;
        }
        $body = @fread($fp, $len);
        if ($close) fclose($fp);
        return $body;
    }

    function http_post($host, $port, $query, $body)
    {
        $fp = fsockopen($host, $port, $errno, $errstr, 1);
        if (!$fp)
        {
            return false;
        }
        $out = "POST ${query} HTTP/1.1\r\n";
        $out .= "Host: ${host}\r\n";
        $out .= "Content-Length: " . strlen($body) . "\r\n";
        $out .= "Connection: close\r\n";
        $out .= "\r\n";
        $out .= $body;
        fwrite($fp, $out);
        $line = trim(fgets($fp));
        $header .= $line;
        list($proto, $rcode, $result) = explode(" ", $line);
        $len = -1;
        while (($line = trim(fgets($fp))) != "")
        {
            $header .= $line;
            if (strstr($line, "Content-Length:"))
            {
                list($cl, $len) = explode(" ", $line);
            }
            if (strstr($line, "Connection: close"))
            {
                $close = true;
            }
        }
        if ($len < 0)
        {
            return false;
        }
        $body = @fread($fp, $len);
        if ($close) fclose($fp);
        return $body;
    }
	
    function http_pget($host, $port, $query)
    {
        $fp = pfsockopen($host, $port, $errno, $errstr, 1);
        if (!$fp)
        {
            return false;
        }
        $out = "GET ${query} HTTP/1.1\r\n";
        $out .= "Host: ${host}\r\n";
        $out .= "Connection: Keep-Alive\r\n";
        $out .= "\r\n";
        fwrite($fp, $out);
        $line = trim(fgets($fp));
        $header .= $line;
        list($proto, $rcode, $result) = explode(" ", $line);
        $len = -1;
        while (($line = trim(fgets($fp))) != "")
        {
            $header .= $line;
            if (strstr($line, "Content-Length:"))
            {
                list($cl, $len) = explode(" ", $line);
            }
            if (strstr($line, "Connection: close"))
            {
                $close = true;
            }
        }
        if ($len < 0)
        {
            return false;
        }
        $body = @fread($fp, $len);
        if ($close) fclose($fp);
        return $body;
    }

    function http_ppost($host, $port, $query, $body)
    {
        $fp = pfsockopen($host, $port, $errno, $errstr, 1);
        if (!$fp)
        {
            return false;
        }
        $out = "POST ${query} HTTP/1.1\r\n";
        $out .= "Host: ${host}\r\n";
        $out .= "Content-Length: " . strlen($body) . "\r\n";
        $out .= "Connection: Keep-Alive\r\n";
        $out .= "\r\n";
        $out .= $body;
        fwrite($fp, $out);
        $line = trim(fgets($fp));
        $header .= $line;
        list($proto, $rcode, $result) = explode(" ", $line);
        $len = -1;
        while (($line = trim(fgets($fp))) != "")
        {
            $header .= $line;
            if (strstr($line, "Content-Length:"))
            {
                list($cl, $len) = explode(" ", $line);
            }
            if (strstr($line, "Connection: close"))
            {
                $close = true;
            }
        }
        if ($len < 0)
        {
            return false;
        }
        $body = @fread($fp, $len);
        if ($close) fclose($fp);
        return $body;
    }
    
    function put($host, $port, $charset='utf-8', $name, $data)
    {
    	$result = $this->http_post($host, $port, "/?charset=".$charset."&name=".$name."&opt=put", $data);
		if ($result == "HTTPSQS_PUT_OK") {
			return true;
		}
		return false;
    }
    
    function get($host, $port, $charset='utf-8', $name)
    {
    	$result = $this->http_get($host, $port, "/?charset=".$charset."&name=".$name."&opt=get");
		if ($result == "HTTPSQS_ERROR" || $result == false) {
			return false;
		}
        return $result;
    }
	
    function status($host, $port, $charset='utf-8', $name)
    {
    	$result = $this->http_get($host, $port, "/?charset=".$charset."&name=".$name."&opt=status");
		if ($result == "HTTPSQS_ERROR" || $result == false) {
			return false;
		}
        return $result;
    }
	
    function view($host, $port, $charset='utf-8', $name, $pos)
    {
    	$result = $this->http_get($host, $port, "/?charset=".$charset."&name=".$name."&opt=view&pos=".$pos);
		if ($result == "HTTPSQS_ERROR" || $result == false) {
			return false;
		}
        return $result;
    }
	
    function reset($host, $port, $charset='utf-8', $name)
    {
    	$result = $this->http_get($host, $port, "/?charset=".$charset."&name=".$name."&opt=reset");
		if ($result == "HTTPSQS_RESET_OK") {
			return true;
		}
        return false;
    }
	
    function maxqueue($host, $port, $charset='utf-8', $name, $num)
    {
    	$result = $this->http_get($host, $port, "/?charset=".$charset."&name=".$name."&opt=maxqueue&num=".$num);
		if ($result == "HTTPSQS_MAXQUEUE_OK") {
			return true;
		}
        return false;
    }
	
    function pput($host, $port, $charset='utf-8', $name, $data)
    {
    	$result = $this->http_ppost($host, $port, "/?charset=".$charset."&name=".$name."&opt=put", $data);
		if ($result == "HTTPSQS_PUT_OK") {
			return true;
		}
		return false;
    }
    
    function pget($host, $port, $charset='utf-8', $name)
    {
    	$result = $this->http_pget($host, $port, "/?charset=".$charset."&name=".$name."&opt=get");
		if ($result == "HTTPSQS_ERROR" || $result == false) {
			return false;
		}
        return $result;
    }
	
    function pstatus($host, $port, $charset='utf-8', $name)
    {
    	$result = $this->http_pget($host, $port, "/?charset=".$charset."&name=".$name."&opt=status");
		if ($result == "HTTPSQS_ERROR" || $result == false) {
			return false;
		}
        return $result;
    }
	
    function pview($host, $port, $charset='utf-8', $name, $pos)
    {
    	$result = $this->http_pget($host, $port, "/?charset=".$charset."&name=".$name."&opt=view&pos=".$pos);
		if ($result == "HTTPSQS_ERROR" || $result == false) {
			return false;
		}
        return $result;
    }
	
    function preset($host, $port, $charset='utf-8', $name)
    {
    	$result = $this->http_pget($host, $port, "/?charset=".$charset."&name=".$name."&opt=reset");
		if ($result == "HTTPSQS_RESET_OK") {
			return true;
		}
        return false;
    }
	
    function pmaxqueue($host, $port, $charset='utf-8', $name, $num)
    {
    	$result = $this->http_pget($host, $port, "/?charset=".$charset."&name=".$name."&opt=maxqueue&num=".$num);
		if ($result == "HTTPSQS_MAXQUEUE_OK") {
			return true;
		}
        return false;
    }
}
?>
