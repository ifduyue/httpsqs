<?php
include_once("httpsqs_client.php");
$httpsqs = new httpsqs;

$message = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
aaaaaaaaaaaaaaaaaaaaaa";

$number = 10000;

/* test queue put */
$start_time = microtime(true);
for ($i=1;$i<=$number;$i++){
    $httpsqs->pput("127.0.0.1", 1218, "utf-8", "command_line_test", $i.$message);
}
$run_time = microtime(true) - $start_time;
echo "Run Time for Queue PUT: $run_time sec, ".$number/$run_time." requests/sec\n";
ob_flush();

/* test queue get */
$start_time = microtime(true);
for ($i=1;$i<=$number;$i++){
    $result = $httpsqs->pget("127.0.0.1", 1218, "utf-8", "command_line_test");
    //echo($result."\n");
}
$run_time = microtime(true) - $start_time;
echo "Run Time for Queue GET: $run_time sec, ".$number/$run_time." requests/sec\n";
?>
