gcc -c -fPIC screenlock.c -limobiledevice -lplist
gcc -c -fPIC pam_screenlock.c -lpam

rm -f screenlock.a; ar cr screenlock.a screenlock.o
ld -shared -o pam_screenlock.so screenlock.o pam_screenlock.o -limobiledevice -lplist -lpam
#ld -Bsymbolic -shared -o pam_screenlock.so screenlock.o archive.a 

nm pam_screenlock.so

sudo mv pam_screenlock.so /lib/security/pam_screenlock.so
sudo chmod 644 /lib/security/pam_screenlock.so

#ls -la /lib/security/


