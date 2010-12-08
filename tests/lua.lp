#begin_lua
function gcd(a, b)
	if a == 0 then   
		return b
	else
		return gcd(b % a, a)
	end
end

function test(x)
	if Val.isFunc(x) then return "func"
	elseif Val.isId(x) then return "id"
	elseif Val.isString(x) then return "string"
	elseif Val.isNum(x) then return "num"
	elseif Val.isSup(x) then return "sup"
	elseif Val.isInf(x) then return "inf"
	end
	return 0
end
#end_lua.

p(2*3*5;2*3*7;2*5*7).
q(X,Y,@gcd(X,Y)) :- p(X;Y), X < Y.

r(1;"1";a;f(a);#supremum;#infimum).
s(X,@test(X)) :- r(X).

