program prog;
var glob : integer;
function f(x : integer) : integer;
begin
	glob := 1;
	f := x * 2
end; 
begin
glob := 2;
f(1);
prog := glob
end