const int base=2; const int global[3]={base,base+1}; int main(){const int local[3]={base,base+1};return local[0]+local[1]+global[2];}
