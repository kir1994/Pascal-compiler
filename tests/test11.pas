program Digit; 
var
num, bol: integer;

begin
num := 3;
case num of
1,2,3 : bol:=1;
4,5,6 : bol:=2; 
7,8,9 : bol:=3;
10,11,12: bol:=4 
else 
bol:=555
end;
Digit:=bol
end