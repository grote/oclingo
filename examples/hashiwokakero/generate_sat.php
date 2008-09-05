#!/usr/bin/php
<?
srand((float)microtime() * 1000000);

$size      = 60;
$maxBridge = 20;
$border    = array(array(1,1));
$bridges   = 0;

function drawX($x, $y, $dir)
{
	global $maxBridge;
	global $border;
	global $size;
	global $vertex;
	global $bridge;
	global $bridges;
	$length = rand(3, $maxBridge + 2);
	for($xn = $x + $dir; $dir > 0 ? $xn < $x + $length && $xn < $size : $xn > $x - $length && $xn > 1; $xn+= $dir)
	{
		if($vertex[$xn][$y] > 0)
			break;
	}
	if(abs($xn - $x) <= 1 || $xn > $size || $xn < 1)
		return false;
	for($i = $x + $dir; $dir > 0 ? $i <= $xn : $i >= $xn; $i+= $dir)
	{
		if($bridge[$i][$y])
			return false;
	}
	for($i = $x + $dir; $dir > 0 ? $i < $xn : $i > $xn; $i+= $dir)
		$bridge[$i][$y] = true;
	$num = rand(1,2);
	$vertex[$x][$y] += $num;
	$vertex[$xn][$y]+= $num;
	$bridges+= $num;
	array_push($border, array($xn ,$y));
	return true;
}

function drawY($x, $y, $dir)
{
	global $maxBridge;
	global $border;
	global $size;
	global $vertex;
	global $bridge;
	global $bridges;
	$length = rand(3, $maxBridge);
	for($yn = $y + $dir; $dir > 0 ? $yn < $y + $length && $yn < $size: $yn > $y - $length && $yn > 1; $yn+= $dir)
	{
		if($vertex[$x][$yn] > 0 || $yn == $size || $yn == 1)
			break;
	}
	if(abs($yn - $y) <= 1)
		return false;
	for($i = $y + $dir; $dir > 0 ? $i <= $yn : $i >= $yn; $i+= $dir)
	{
		if($bridge[$x][$i])
			return false;
	}
	for($i = $y + $dir; $dir > 0 ? $i < $yn : $i > $yn; $i+= $dir)
		$bridge[$x][$i] = true;
	$num = rand(1,2);
	$vertex[$x][$y] += $num;
	$vertex[$x][$yn]+= $num;
	$bridges+= $num;
	array_push($border, array($x ,$yn));
	return true;
}
function draw()
{
	global $border;
	while(count($border) > 0)
	{
		$remove = rand(0, count($border) - 1);
		$x = $border[$remove][0];
		$y = $border[$remove][1];
		
		$todo = array(1,2,3,4);
		shuffle($todo);
		foreach($todo as $val)
		{
			switch($val)
			{
				case 1:
				{
					if(drawX($x, $y, 1))
						continue 2;
					break;
				}
				case 2:
				{
					if(drawX($x, $y, -1))
						continue 2;
					break;
				}
				case 3:
				{
					if(drawY($x, $y, -1))
						continue 2;
					break;
				}
				case 4:
				{
					if(drawY($x, $y, 1))
						continue 2;
					break;
				}
			}
		}
		for($i = $remove + 1; $i < count($border); $i++)
			$border[$i - 1] = $border[$i];
		array_pop($border);
	}
}

draw();

print "% puzzle with $bridges bridges\n";
print "% instance is sat\n\n";

print "start(1,1).\n";
for($x = 1; $x <= $size; $x++)
{
	$newline = false;
	for($y = 1; $y <= $size; $y++)
	{
		$n = (int)($vertex[$x][$y]);
		assert($n <= 8);
		if($n > 0)
		{
			$newline = true;
			print "vertex($x, $y, $n). ";
		}
	}
	if($newline)
		print "\n";
}

?>
