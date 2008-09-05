#!/usr/bin/php
<?
$size = 100;

$bridgeNum = 100000;
$maxTrys   = 100000;
$maxBridge = 2;
$i = 0;
$old_i = 0;
while($i < $bridgeNum)
{
	if($i != $old_i)
	{
		$trys = 0;
		$old_i = $i;
	}
	else
	{
		$trys++;
	}
	if($trys > $maxTrys)
		break;
	$t1 = rand(1, $size);
	$t2 = rand(1, $size);
	do
	{
		$t3 = rand(1, $size);
	}
	while($t2 == $t3 || abs($t3 - $t2) > $maxBridge);
	if(rand(1,2) == 2)
	{
		$x = $t1;
		if($t2 > $t3)
		{
			$y1 = $t3;
			$y2 = $t2;
		}
		else
		{
			$y2 = $t3;
			$y1 = $t2;
		}
		for($j = $y1+1; $j < $y2; $j++)
		{
			if($vertices[$x][$j] > 0)
			{
				$y2 = $j;
				break;
			}
		}
		if($y1 + 1 == $y2)
			continue;
		for($j = $y1 + 1; $j < $y2; $j++)
			if($bridges[$x][$j] > 0)
				continue 2;
		$i++;
		$num = rand(1,2);
		assert($num == 1 || $num == 2);
		for($j = $y1+1; $j < $y2; $j++)
		{
			$bridges[$x][$j] = $num;
		}
		if($vertices[$x][$y2] > 0)
			assert($bridges[$x][$y2] == 0);
		if($vertices[$x][$y1] > 0)
			assert($bridges[$x][$y1] == 0);
		assert($y2 > $y1 + 1);
		$vertices[$x][$y1]+= $num + $bridges[$x][$y1];
		$vertices[$x][$y2]+= $num + $bridges[$x][$y2];
		$bridges[$x][$y1] = 0;
		$bridges[$x][$y2] = 0;	
	}
	else
	{
		$y = $t1;
		if($t2 > $t3)
		{
			$x1 = $t3;
			$x2 = $t2;
		}
		else
		{
			$x2 = $t3;
			$x1 = $t2;
		}
		for($j = $x1+1; $j < $x2; $j++)
		{
			if($vertices[$j][$y] > 0)
			{
				$x2 = $j;
				break;
			}
		}
		if($x1 +1 == $x2)
			continue;
		for($j = $x1 + 1; $j < $x2; $j++)
			if($bridges[$j][$y] > 0)
				continue 2;
		$i++;
		$num = rand(1,2);
		for($j = $x1+1; $j < $x2; $j++)
		{
			$bridges[$j][$y] = $num;
		}

		assert($x2 > $x1 + 1);
		$vertices[$x1][$y]+= $num + $bridges[$x1][$y];
		$vertices[$x2][$y]+= $num + $bridges[$x2][$y];
		$bridges[$x1][$y] = 0;
		$bridges[$x2][$y] = 0;
	}
}

print "% puzzle with $i bridges\n";
print "% instance which is most probably unsat\n\n";

for($x = 1; $x <= $size; $x++)
{
	$newline = false;
	for($y = 1; $y <= $size; $y++)
	{
		$n = (int)($vertices[$x][$y]);
		assert($n <= 8);
		if($n > 0)
		{
			$newline = true;
			if(!$start)
			{
				print "start($x, $y).\n";
				$start = true;
			}
			print "vertex($x, $y, $n). ";
		}
	}
	if($newline)
		print "\n";
}

?>
