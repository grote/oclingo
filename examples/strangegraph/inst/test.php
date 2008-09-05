#!/usr/bin/php
<?

$edgeList;
$nodeList;
$nodeIn;
$nodeOut;

$edges=30;
$n = 3;
$nodes = 1;

$x1 = 0;
$y1 = 0;

$nodeList[$x1][$y1] = $nodes++;
$nodeIn[$x1][$y1] = 0;

while($i < $edges)
{
	$x2 = rand(0, $n-1);
	$y2 = rand(0, $n-1);
	if(!$edgeList[$x1][$y1][$x2][$y2])
	{	
		$edgeList[$x1][$y1][$x2][$y2] = true;
		if(rand(1,4) != 1 || !$nodeOut[$x1][$y1] || !$nodeIn[$x2][$y2])
		{
			$nodeOut[$x1][$y1]++;
			if(!$nodeIn[$x2][$y2] || rand(1,4) == 1)
				$nodeIn[$x2][$y2]++;
		}
		$i++;
		$x1 = $x2;
		$y1 = $y2;
		if(!$nodeList[$x2][$y2])
			$nodeList[$x2][$y2] = $nodes++;
	}
}
$nodeOut[$x2][$y2] += 0;

foreach($edgeList as $x1 => $edgeListY1)
{
	foreach($edgeListY1 as $y1 => $edgeListX2)
	{
		$n1 = $nodeList[$x1][$y1];
		foreach($edgeListX2 as $x2 => $edgeListY2)
		{
			foreach($edgeListY2 as $y2 => $dummy)
			{
				$n2 = $nodeList[$x2][$y2];
				print "edge($n1, $n2). ";
			}
		}
		print "\n";
	}
}

foreach($nodeList as $x => $nodeListY)
{
	foreach($nodeListY as $y => $n)
	{
		$in = $nodeIn[$x][$y];
		$out = $nodeOut[$x][$y];
		print "node($n, $in, $out). \n";
	}
}

echo "start(1)."

?>
