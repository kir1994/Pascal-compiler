program prog;
var
    i: integer;
function 
	f1() : integer;
	function 
		f2(var x : integer) : integer;
		begin
			x := 1
		end;
	begin
		f2(i)
	end;
begin
	i := 0;
	f1();
	prog := i
end