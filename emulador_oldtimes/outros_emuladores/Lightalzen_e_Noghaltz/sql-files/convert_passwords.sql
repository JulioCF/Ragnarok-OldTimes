# conversor de passwords para criptografia MD5
# ATENÇÃO! 
# Não é possível retornar ao valor inicial após transformar em MD5!!!

UPDATE `login` SET `user_pass`=MD5(`user_pass`);
