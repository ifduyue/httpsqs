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
