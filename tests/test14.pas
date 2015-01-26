program repeatuntil; 
var
sum, a: integer; 
begin
sum:=0; 
a:=10; 

repeat
sum:=sum+a; 
a:=a-2
until a<>0;

repeatuntil:=sum
end