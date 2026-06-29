# Proje Raporu (TR): LLM Destekli Statik Kod Analizi
### Wang et al. (2025) survey'i üzerine ampirik bir vaka çalışması

> Bu belge **senin için**: tam anlaman, savunman ve Pazartesi hocayla
> konuşman için. Akademik teslim metni ayrıca Almanca (`seminararbeit.md`).
> Deney sonuçları henüz yok → ilgili yerler `[DOLDUR]` ile işaretli.

---

## 1. Tek cümlede proje
Büyük dil modellerini (LLM) bir "sihirbaz" gibi değil, **ham metin okuyan
kusurlu bir asistan** gibi ele alıp, Wang et al.'ın survey'inde sıralanan
zaafların (kararsızlık, halüsinasyon/false positive, token/ölçek sınırı)
**gerçekten ne zaman ortaya çıktığını** tek bir model (Groq) üzerinde,
etiketli test dosyalarıyla ve klasik bir araçla (cppcheck) karşılaştırarak
**ölçüyoruz.**

## 2. Neden bu açı? (Hocaya satılan şey)
- Survey'in **§ III**'ü LLM'lerin statik analizde nasıl kullanıldığını anlatır
 (bizim *yöntem* tarafımız).
- Survey'in **§ VI-A**'sı (Discussion → Challenges) LLM'lerin zaaflarını
 sıralar (bizim *asıl vurgumuz*). Biz tam bu zaafları hedef alıyoruz.
- Duruşumuz: LLM bir orakl değil. Bunu **kanıtlamaya** değil, **ne zaman
 doğru/yanlış** olduğunu görmeye çalışıyoruz. Bu, naif "LLM kötü/iyi"
 söyleminden daha olgun ve savunulabilir.

> Not, numaralandırma: Planımızdaki "3. Bölüm" = survey'in **§ III**
> (statik analiz), "6. Bölüm" = **§ VI** (Discussion/Challenges). Doğru.

## 3. Araştırma soruları ve paper'daki dayanağı
Her RQ doğrudan survey'in § VI-A'sındaki bir cümleye bağlı (alıntılar birebir):

| RQ | Soru | Paper dayanağı (§ VI-A) |
|---|---|---|
| **RQ1** | Aynı girdi + temp=0'da çıktı koşudan koşuya değişiyor mu? | "LLMs **are non-deterministic and may produce varying outputs for identical inputs**" |
| **RQ2** | LLM zararsız "yem"lerde hayali açık uyduruyor mu? | "prone to **hallucinations, generating fabricated information**" |
| **RQ3** | Naif vs yapılandırılmış prompt sonucu değiştiriyor mu? | "effectiveness … **relies on prompt engineering**" |
| **RQ4** | Büyük repoda doğruluk düşüyor mu, slicing yardım ediyor mu? | "**inherent token limits** … restrict … making **scalability** a challenge" |
| **RQ5** | LLM `x²=33`'ü saf matematik mi, makine semantiği mi okuyor? | semantik sonda, formal/SMT'nin parladığı, LLM'in kaydığı yer |

Bonus dayanaklar (raporda kullan):
- Mantık hataları: "LLMs **struggle to analyze logic vulnerabilities involving
 intricate control flows**" → Java backdoor vakanı destekler.
- Komplementerlik: LLM'ler "**overcome the rule-based limitations of
 traditional tools** … identifying nuanced vulnerabilities … missed by
 automated methods" (§ VI-B, Project Naptime) → LLM'in cppcheck'i geçebileceği
 yer (semantik açıklar).

## 4. Sistem mimarisi: her dosya ne yapıyor
Devasa bir mimari **yok**; bilinçli olarak sade:

```
src/analyzer.py     Postacı. Kodu RAW TEXT okur → Groq'ye atar → N kez
                    çalıştırır → her çıktıyı results/raw/*.json olarak
                    model/temp/tarih/prompt-hash ile kaydeder (reproducibility).
src/slicer.py       Hafif HEURİSTİK slicer (gerçek veri/kontrol akışı DEĞİL).
                    Bir fonksiyon + onu çağıran/çağrılanları toplar. Büyük-repo
                    deneyi için: "tüm repo" vs "sadece ilgili fonksiyonlar".
src/prompts/        İki Almanca prompt: naif ("hataları bul") ve yapılandırılmış
                    (CWE+satır+şiddet ister, "açık yoksa yok de" → FP'yi ölçer).
src/config.py       Model adı, temperature (0.0), koşu sayısı.
src/list_models.py  API key'inin desteklediği modelleri listeler (404 olursa).
testcases/          Etiketli test dosyaları (gerçek bug + tuzak).
testcases/ground_truth.yaml   "Doğru cevap" tablosu → TP/FP/FN sayımı.
baselines/run_cppcheck.sh     Klasik araç + süre ölçümü.
repo_experiment/    Büyük repo + slicing deneyinin planı.
results/raw/        Ham LLM çıktıları (commit edilir → tekrar üretilebilir).
docs/               Bu rapor (TR) + seminararbeit.md (DE) + survey notları.
```

## 5. Test vakaları: ne, neden
Hepsinin "doğru cevabı" `ground_truth.yaml`'da; böylece değerlendirme "his"
değil, **sayılabilir** (kaç gerçek bug bulundu = TP/FN, kaç hayali açık = FP).

**`01_buffer_overflow_memleak.c`**, Gerçek: buffer overflow (CWE-120, satır
21 `strcpy`), memory leak (CWE-401, satır 33 `malloc` ama `free` yok). Tuzak
yok → temiz tespit beklenir; klasik aracın (cppcheck) ev sahası.

**`02_signed_unsigned.c`**, Gerçek: signedness hatası (CWE-195, negatif `int`
→ `size_t` → `memcpy` taşması), 8-bit integer overflow (CWE-190, `uint8_t`
çarpım mod 256). Semantik sonda: `x²=33`. **Doğrulandı:** 8-bit unsigned'da
çözümler **x = 17, 111, 145, 239** (17²=289=256+33). Reel sayıda çözüm yok →
LLM "çözümsüz" derse makine semantiğini kaçırmış demektir; SMT/Z3 ise anında
bulur. Bu, seminerin formal-metot çekirdeğine bağlanan en keskin örnek.

**`AuthManager.java`**, Gerçek: `==` ile String karşılaştırma (CWE-597, satır
22), "test" içeren kullanıcı adına backdoor (CWE-912, satır 29, mantık hatası,
linter'ın zor yakaladığı). Tuzak: `dummyPassword` (satır 17, kullanılmıyor) →
LLM "hardcoded secret" derse **false positive** (halüsinasyon kanıtı).

**`01_command_injection.py`** (opsiyonel), Gerçek: `shell=True` komut
enjeksiyonu (CWE-78), `eval` (CWE-95). Tuzak: `unused_api_token`.

## 6. Metodoloji ve dürüst çerçeveleme
- **Tek model (Groq), temp=0** → kontrollü. (Hocayla "tek model" konuşuldu.)
- **N-kez çalıştırma** → RQ1'i *ölçer* (iddia etmez): bir bug kaç koşuda
 bulundu (örn. 7/10).
- **TP/FP/FN** → ground_truth ile karşılaştırma.
- **İki prompt** → RQ3 (prompt duyarlılığı; bu bile bir bulgu).
- **Baseline** → cppcheck (C için). Süre karşılaştırması.
- **Bilinçli "vaka çalışması" dili** → küçük n; "validasyon" DEME. Formal-metot
 hocası "validasyon" kelimesine takılır, "case study"ye takılmaz.

## 7. İlk deney sonuçları (gerçek veri)
Model: Groq `llama-3.3-70b-versatile`, temp=0, her (dosya,prompt) için 10 koşu.
Değerlendirme: `src/score.py` (heuristik + elle teyit).

**RQ1, Kararsızlık (en güçlü bulgu).** temp=0 olmasına rağmen:
- AuthManager (structured): **10 koşunun 8'i farklı CWE kümesi**; uzunluk 809 ila 2216.
- 02_signed_unsigned: 2/10 farklı CWE kümesi; AuthManager-naive ve buffer_overflow
 içerikçe stabil ama uzunluk yine savruluyor.
- **Somut halüsinasyon:** aynı `==` hatasına koşulara göre CWE-597 / 304 / 303 /
 hatta 757 (alakasız "zayıf şifreleme") atadı. Doğru yeri buluyor,
 sınıflandırmayı uyduruyor → survey § VI-A "non-deterministic + hallucinations".

**Gerçek bug tespiti (TP).** Tüm gömülü bug'lar her iki promptta da **10/10**
bulundu (buffer overflow, memory leak, signedness, int-overflow, == hatası,
backdoor). → Küçük dosyada model güçlü; "zaaflar ölçeğe bağlı" çerçeveni doğrular.

**KRİTİK düzeltme, tuzak testi kör değildi.** Test dosyalarındaki Türkçe yorumlar
cevabı sızdırıyordu: model run1'de senin `// TUZAK (False Positive Testi)`
yorumunu birebir okuyup "ein 'Tuzak'... um False Positives zu testen" yazdı.
Yani RQ2 kontamine. Çözüm: yorumsuz `testcases/clean/` dosyaları oluşturuldu;
RQ2'yi BUNLARDA yeniden koşacaksın.

**RQ2, halüsinasyon/false positive (clean vs kontamine).** Zararsız
`dummyPassword` tuzağının ele alınışı (her biri 10 koşu):

| Sürüm | Prompt | anıldı | formal açık (FP) | reddedildi |
|---|---|---|---|---|
| kontamine | structured | 10/10 | 0/10 | 10/10 |
| kontamine | naive | 7/10 | 0/10 | 7/10 |
| **clean (kör)** | **structured** | 6/10 | **6/10** | 2/10 |
| clean (kör) | naive | 2/10 | 0/10 | 1/10 |

→ **Asıl bulgu:** yorum-sızıntısı halüsinasyonu maskelemiş. Yorum varken tuzak
hep doğru elendi (0 FP); kör testte structured prompt `dummyPassword`'ü 6/10
koşuda gerçek açık (CWE-521/798) diye uydurdu + ekstra şüpheli bulgular ekledi
(hardcoded "ADMIN" rolü, çıktı satırları). naive kör testte de sağlam (0 FP).
Gerçek bug'lar (==, backdoor) kör testte de 10/10 bulundu. Geçerli bir FP testi
KÖR olmalı, bu metodolojik farkındalık hocaya çok iyi görünür.

**RQ3, Prompt duyarlılığı.** Aynı kod/model, farklı prompt: structured hem
kararsızlığı uçuruyor (8/10 vs 1/10) hem tuzağı listeleme eğiliminde; naive
doğru elliyor. Prompt mühendisliği gerçek bir confounder.

**Önceki abartının düzeltmesi:** "structured 10/10 CWE-798 dedi" demiştim; doğrusu
yukarıdaki nüanslı tablo, ve bu hali daha inandırıcı.

**Eksikler:** (1) clean dosyalarda RQ2'yi yeniden koş, (2) RQ4 büyük repo+slicing,
(3) RQ5 için x²=33'ü doğrudan soran özel prompt, (4) `bash baselines/run_cppcheck.sh`.

## 8. Beklenen sonuçlar + en kritik savunma
**Risk:** Oyuncak dosyalar küçük; modern Groq bunları temiz çözebilir,
tuzağa düşmeyebilir. O zaman "zaafları gösteremedim" gibi görünür.

**Savunma (ezberle):** Eğer LLM küçük dosyalarda başarılıysa bu tezi
*çürütmez*, **paper'ı doğrular**: survey'in dediği gibi sorunlar
**ölçek/bağlam** bağımlıdır (büyük, gerçek kod tabanları). Bu yüzden RQ4
(büyük repo + slicing) var. Yani deney ne çıkarsa çıksın anlatın tutarlı:
- Küçük dosyada başarılı → "zaaflar ölçeğe bağlı" (RQ4'e köprü).
- Küçük dosyada bile hata/halüsinasyon → "zaaf düşük karmaşıklıkta bile var".

## 9. cppcheck vs LLM: komplementerlik (gerçek sonuç)
cppcheck 2.21 iki C dosyasını 0,08 saniyede analiz etti (deterministik).

| Bug (CWE) | cppcheck | LLM (k/N) |
|---|---|---|
| Buffer Overflow (120) | bulamadı | 10/10 |
| Memory Leak (401) | buldu (error/memleak) | 10/10 |
| Signedness (195) | bulamadı | 10/10 |
| Int-Overflow (190) | bulamadı | 10/10 |

cppcheck 4 gerçek bug'dan sadece 1'ini buldu, yani memory leak'i (klasik
alanı). Buffer overflow, signedness ve integer overflow'u kaçırdı (false
negative), buna karşılık güvenlik açığı olmayan stil/uyarı gürültüsü üretti
(unusedFunction, unreadVariable, staticFunction). LLM ise dördünü de buldu
(yüksek recall) ama kararsız ve halüsinasyonlu. Yani araçlar birbirinin yerine
değil, birbirini tamamlar: cppcheck hızlı ve kesin ama dar kapsamlı, LLM geniş
ama güvenilmez. Tam olarak "kim nerede kazanıyor" tezi.

## 10. Sınırlamalar (dürüstçe yaz: güç kazandırır)
Küçük n (vaka çalışması); tek model/sağlayıcı; model davranışı zamanla
değişir (bu yüzden model/tarih/temp kaydediliyor); slicer bir heuristik
(gerçek slice değil); prompt seçimi sonucu etkiler.

## 11. Pazartesi konuşma planı (5 dakikalık akış)
1. **Çerçeve (30 sn):** "LLM'i orakl değil, ham metin okuyan kusurlu asistan
  olarak ele alıyorum; Wang § VI-A'daki zaafların ne zaman gerçekleştiğini
  ölçüyorum."
2. **Bağlantı (30 sn):** § III = yöntem, § VI-A = zaaflar; RQ1 ila RQ5 her biri
  bir survey cümlesine bağlı (tabloyu göster).
3. **Somut kanıt (1 dk):** `x²=33` örneği, reel'de çözümsüz, 8-bit'te
  {17,111,145,239}; SMT bulur, LLM kayabilir. (Hocanın kendi örneği!)
4. **Tasarım (1 dk):** 3 parça (test+ground truth / analyzer / rapor) +
  cppcheck baseline + büyük-repo slicing deneyi. Devasa mimari yok.
5. **Dürüstlük (1 dk):** vaka çalışması, küçük n; "deney tezi çürütürse"
  savunması (Bölüm 8); slicer heuristik.
6. **Soru (1 dk):** "Ağırlığı paper analizinde mi, proof-of-concept'te mi
  istersiniz? Diğer paper'lara (VeCoGen, refinement calculus) ne kadar
  bağlayayım?" → topu hocaya at, yön ver.

## 12. Senin yapılacaklar listen
- [ ] `py -m pip install -r requirements.txt`
- [ ] `.env`'e Groq key (yaptın). Test: `py src/analyzer.py --file testcases/c/01_buffer_overflow_memleak.c --runs 1`
- [ ] 4 dosyayı 10'ar koşu çalıştır → `results/raw/` dolsun
- [ ] Çıktıları `ground_truth.yaml` ile karşılaştır → TP/FP/FN say
- [ ] Tabloları hem bu raporda hem `seminararbeit.md`'de doldur
- [ ] Büyük repo seç (C snake/tetris), bug göm, `repo_experiment/` deneyi
- [ ] Windows'ta: yarım `.git` ve `__pycache__` klasörlerini sil → `git init`
