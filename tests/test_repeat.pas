program prog;
const n=10;
var
    i: real;
    sum: integer;
begin
	i := n;
	sum := 0;
    repeat
	begin
        sum:= sum+i;
		i := i - 1
	end
	until i <> 0;
	prog:=sum
end