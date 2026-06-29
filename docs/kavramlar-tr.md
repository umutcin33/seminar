# Kavramlar ve Araştırma Soruları (TR)

> Senin için açıklayıcı not. Sunumda ve savunmada bu kavramları rahatça
> anlatabilmen için. Akademik teslim metni Almanca (`seminararbeit.md`).

## 1. Projenin mantığı tek paragrafta
Bir LLM'i (Groq üzerinde Llama 3.3 70B) statik kod analizcisi gibi kullanıyoruz.
Statik analiz, kodu çalıştırmadan, sadece metnine bakarak hata aramak demektir.
Biz LLM'e kodu düz metin olarak veriyoruz, yani AST, CFG gibi yapı çıkarmıyoruz.
Amaç, Wang et al. survey'inin zaaflar bölümündeki (§ VI-A) iddiaları gerçek
deneyle sınamak. LLM'i sihirli bir araç gibi değil, ham metin okuyan kusurlu
bir asistan gibi değerlendiriyoruz.

## 2. Değerlendirme metrikleri (en önemli kısım)
Elimizde her dosya için "doğru cevap" anahtarı var (`ground_truth.yaml`).
Modelin her bulgusunu bu anahtara göre dört kategoriden birine koyuyoruz.

True Positive (TP), doğru pozitif: bizim koyduğumuz gerçek bir hatayı bulması.
İstediğimiz şey.

False Negative (FN), yanlış negatif: gerçek bir hatayı kaçırması. Kötü, çünkü
açığı gözden kaçırıyor.

False Positive (FP), yanlış pozitif: aslında hata olmayan bir şeyi açık diye
raporlaması. Kötü, çünkü gürültü ve halüsinasyon üretiyor. Tuzaklarımız tam
bunu ölçüyor.

True Negative (TN), doğru negatif: zararsız bir şeyi doğru biçimde
işaretlememesi. İstediğimiz şey. Tuzağı "bu açık değil" diye geçmesi bir TN'dir.

Neden FP bu kadar önemli: pratikte statik analiz araçlarını öldüren şey yanlış
alarmlardır. Çok fazla FP olursa geliştirici araca güvenmeyi bırakır. Buna
alarm yorgunluğu (alert fatigue) denir.

## 3. `==` nedir (Java'daki gerçek hata)
İlgili satır: `if (actualRole == providedRole)`. Java'da `==` operatörü
nesneler için "bunlar bellekte aynı nesne mi" diye bakar, yani referans
karşılaştırır. `.equals()` ise "içerikleri aynı mı" diye bakar, yani değer
karşılaştırır. `String` bir nesne olduğu için, içeriği aynı iki String bellekte
farklı nesneler olabilir ve `==` yanlış sonuç verebilir. Doğrusu
`actualRole.equals(providedRole)`. Bu klasik bir Java hatasıdır.

## 4. `dummyPassword` nedir (tuzak)
İlgili satır: `String dummyPassword = "default_placeholder";`. Bu değişken
tanımlanıyor ama hiçbir yerde kullanılmıyor, yani zararsız ölü kod. İsmini
bilerek "password" içerecek şekilde koyduk. Yüzeysel bir analizci "password" +
sabit metin görünce "koda gömülü parola, güvenlik açığı" diye bağırabilir.
Oysa gerçekte hiçbir şey. Model bunu açık derse yanlış pozitif (halüsinasyon)
üretmiş olur. Bu tuzak, modelin davranışı mı analiz ettiğini yoksa korkutucu
isme mi takıldığını ölçer.

## 5. CWE nedir
CWE, Common Weakness Enumeration, yani MITRE'nin tuttuğu standart yazılım
zayıflık kataloğu. Her zayıflık türünün bir numarası var. Ortak dil sağlar,
herkes aynı hata türüne aynı numarayla atıfta bulunur. Model çıktısında
"CWE-401" yazınca "bu, standart katalogdaki bellek sızıntısıdır" diyor.

| CWE | Anlamı | Bizim projede |
|---|---|---|
| CWE-120 | Boyut kontrolsüz buffer kopyalama (buffer overflow) | C dosya 1, `strcpy` |
| CWE-401 | Belleğin serbest bırakılmaması (memory leak) | C dosya 1, eksik `free` |
| CWE-195 | Signed'dan unsigned'a hatalı dönüşüm | C dosya 2, negatif `int` |
| CWE-190 | Integer overflow, wraparound | C dosya 2, `uint8_t` çarpım |
| CWE-597 | String karşılaştırmada yanlış operatör (`==`) | Java, gerçek bug 1 |
| CWE-595 | İçerik yerine nesne referansını karşılaştırma | Java `==` için de geçerli |
| CWE-912 | Gizli işlevsellik (backdoor) | Java, gerçek bug 2 |
| CWE-798 | Koda gömülü kimlik bilgisi | model `dummyPassword` için uydurdu (FP) |
| CWE-521 | Zayıf parola gereksinimleri | model `dummyPassword` için uydurdu (FP) |
| CWE-78 | OS komut enjeksiyonu | Python, `shell=True` |
| CWE-95 | Eval enjeksiyonu | Python, `eval` |

## 6. Bir çıktıyı nasıl okuyoruz (örnek)
Kör (clean) structured Java koşusunda model şu beş şeyi buldu: hardcoded
"ADMIN" rolü (CWE-798), `dummyPassword` (CWE-521), `==` karşılaştırması
(CWE-595), "test" arka kapısı (CWE-304), çıktı satırı (CWE-259). Doğru cevap
ise iki gerçek bug (`==` ve backdoor) artı bir tuzak (dummyPassword) idi.
Yorumu: `==` ve backdoor bulundu, yani 2 TP ve 0 FN. dummyPassword'ü açık
saydı, yani FP, tuzağa düştü. Ekstra iki bulgu (ADMIN rolü, çıktı) gri bölge,
yani fazladan üretilmiş şüpheli bulgular.

## 7. Araştırma soruları (RQ) tek tek
Her RQ doğrudan survey'in bir cümlesine bağlı, yani deney rastgele değil,
paper'ı sınayan bir deney.

### RQ1, Kararsızlık (Non-Determinism)
Soru: Aynı kodu, aynı prompt'la, temperature sıfırda defalarca çalıştırınca
model hep aynı cevabı mı veriyor? temperature, modelin rastgeleliğini ayarlayan
parametredir; sıfır, en deterministik ayardır ve normalde hep aynı cevap
beklenir. Neden önemli: bir araç aynı girdiye farklı cevap veriyorsa ona
güvenemezsin, sonuç tekrarlanabilir olmaz. Survey dayanağı: "non-deterministic,
may produce varying outputs for identical inputs". Nasıl ölçüyoruz: aynı dosyayı
10 kez çalıştırıp çıktı uzunluklarını ve bulunan CWE kümelerini karşılaştırıyoruz.
İlk sonuç: Java structured'da 10 koşunun 8'i farklı CWE kümesi verdi.

### RQ2, Halüsinasyon ve Yanlış Pozitif (False Positive)
Soru: Model olmayan bir açığı var gibi gösteriyor mu? Tuzak (dummyPassword)
bunu ölçer. Neden önemli: yanlış alarm fazlalığı aracı kullanılamaz hale getirir.
Survey dayanağı: "hallucinations, generating fabricated information". Nasıl
ölçüyoruz: tuzağa kaç koşuda açık dedi. İlk sonuç: kör testte structured prompt
6/10 koşuda tuzağı açık sandı.

### RQ3, Prompt Duyarlılığı
Soru: Aynı kod ve model, sadece soruyu (prompt) değiştirince sonuç değişiyor mu?
İki prompt var: naif ("hataları bul") ve yapılandırılmış (CWE, satır, şiddet
iste). Neden önemli: sonuç prompt'a aşırı bağlıysa araç güvenilmez. Survey
dayanağı: "relies on prompt engineering". İlk sonuç: yapılandırılmış prompt hem
kararsızlığı artırdı hem tuzağı açık olarak listeleme eğilimi gösterdi.

### RQ4, Bağlam ve Slicing
Soru: Küçük dosyada iyi olan model, büyük bir repoda da iyi mi? Sadece ilgili
fonksiyonları vermek (slicing) yardım ediyor mu? Neden önemli: gerçek kod
büyüktür ve modelin token (dikkat) sınırı vardır. Survey dayanağı: "inherent
token limits, scalability". Durum: henüz yapılmadı, planı `repo_experiment/`
içinde.

### RQ5, Makine Semantiği
Soru: Model `x²=33`'ü saf matematik gibi mi (çözüm yok) yoksa makine
aritmetiği gibi mi (8 bit, mod 256, çözüm var) okuyor? Neden önemli: gerçek C
hatalarının çoğu (overflow, signedness) tam buradadır; formal metotlar (SMT,
Z3) burada kesin cevap verir, LLM kayar. Durum: gcc ile çözümler doğrulandı
(17, 111, 145, 239); modeli doğrudan soran özel prompt henüz yapılmadı.

## 8. clean ile kontamine ayrımı
İlk test dosyalarının yorumları cevabı sızdırıyordu, model yorumu okuyup tuzağı
tanıyordu. Bu yüzden yorumsuz (kör) bir set ürettik, `testcases/clean/`. İki
set var: `testcases/` yorumlu (kontamine), `testcases/clean/` yorumsuz (kör).
Geçerli bir yanlış pozitif testi kör olmalı.
