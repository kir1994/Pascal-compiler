program whiledo; 
var
eps:real; 
x: real; 
sum: real;
y: real; 
i: integer; 
begin
x:=12;
eps:=1;
y:=2; 
sum:=y;
i :=0; 
while y>=eps do 
begin  
i:=i+1; 
y:=y*x/i; 
sum:=sum+y 
end;   
whiledo:=sum
end