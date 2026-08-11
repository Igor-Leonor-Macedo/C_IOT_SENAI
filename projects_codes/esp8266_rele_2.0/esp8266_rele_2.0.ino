/*
 * =====================================================
 *  Controle de 3 Relés via ESP8266 - Aula de IoT
 *  Curso Técnico em Automação Industrial - Maria de Fátima
 * =====================================================
 *
 * CONFIGURAÇÃO DE HARDWARE:
 *  - ESP8266 (NodeMCU ou similar)
 *  - Relé 1: pino D1 (GPIO5)
 *  - Relé 2: pino D2 (GPIO4)
 *  - Relé 3: pino D3 (GPIO0)
 *    (A maioria dos módulos relé é ativo em LOW)
 *
 * BIBLIOTECAS NECESSÁRIAS (Arduino IDE):
 *  - ESP8266WiFi      (já inclusa no core do ESP8266)
 *  - ESP8266WebServer (já inclusa no core do ESP8266)
 *
 * COMO INSTALAR O CORE DO ESP8266 NA ARDUINO IDE:
 *  File > Preferences > Additional Boards Manager URLs:
 *  http://arduino.esp8266.com/stable/package_esp8266com_index.json
 *  Depois: Tools > Board > Boards Manager > buscar "esp8266"
 *
 * COMO USAR:
 *  1. Altere SSID e PASSWORD abaixo para sua rede Wi-Fi
 *  2. Faça upload para o ESP8266
 *  3. Abra o Monitor Serial (115200 baud) para ver o IP
 *  4. Acesse o IP no navegador do notebook
 * =====================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ─────────────────────────────────────────────
//  CONFIGURAÇÕES — altere aqui
// ─────────────────────────────────────────────
const char* ssid     = "POCO X3 Pro";
const char* password = "Igor_37#";

// Pinos dos relés (NodeMCU)
const int PINOS_RELE[3] = { 5, 4, 0 };   // GPIO5, GPIO4, GPIO0

// true  → relé ativo em LOW  (mais comum, módulos azuis)
// false → relé ativo em HIGH
const bool RELE_ATIVO_LOW = true;
// ─────────────────────────────────────────────

ESP8266WebServer server(80);
bool estadoRele[3] = { false, false, false };

// ─── Logo SENAI em Base64 ──────────────────────
// Cole aqui a string Base64 do logo (sem o prefixo "data:image/png;base64,")
// Gere a string em: https://www.base64-image.de/  ou via terminal:
//   base64 -i logo_senai.png -o logo_base64.txt   (Linux/macOS)
// Recomenda-se PNG pequeno (até ~30-50KB) com fundo transparente.
const char LOGO_SENAI_B64[] PROGMEM =
  "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAbQAAABuCAYAAABP5ua7AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAG8NJREFUeNrsXb1y47qSxjm18Y5OdjPTT2D6CUyHJxo5uRtazrfK9hPIegLbVTe3HO5NLEc3NP0EQz+BOdnJlrMvcJfQNKwWBuAfAJKSv69K5R+RQOOvP3SjAQgBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMD+4rfPUMj/PP1HVP6I2L+K/3v572xHZE9M/y/lT9F9AQAA9ozQSqU/KX/E5Ucq/6Pyo/6etEhGElxBnzf6Xxqa/Iiw5OeASLet3BwFlUMYypGX5cg7kH9tnr7rp5ShUx2EJHmSacr6V1LzSk6fV9kmpWyrULL99eeXtm22Jeff/vUjDyHXf/3976o/x1r/zP/nn//M+9ANJMOM8lwGzkd+ijKfrOL7rPy+8JSn7IdXocvWQI6boWVQ+I8dJzHZUb82UC5NELPfp/RzTnkpwstIQaVNyKGGxM4pn4nHatEVrV6O0yqlX34v6/OhQ3nuynSvPbXrjZK3w7vXpRx3nvvYFbVVW8JQCiyhtKQSuy8/sq4KXzISmX1znAT94VnBSVmeKsal7IOnPamKhPrTkj6h8EB52erzkvrShUc5YipbGrhsVW2dDC0Dx+87SGRR+ZGd53/Lz60nMmvaeZTCfy9lmHYhsvLzUv76QmlNeq6+uEK2aRcyI1zZXKMd8NWR0H31M6l83mmwRh6SnFBa38ja8wXXfhSiDyrlLid9i/JzRgR2QX8veuzzqnxvgfNRfWRSKvlZxdjLAuT5OqBKjkcgw+5ZaDRbvqUBPAbIhly1kP+WZmhDomownTumfU6ztGCk2wDpDvQzqYReynwOPVlqB47vpwHKqCZ7x77ca11R5i8t9ruQeZBFGmmTsqXBUhQmd6RD2UJbnW36Xw5Ca65kYrJqJiMSK2so+4Rkj0cus6t8s7KsCw+uWBf4GFR9tJVyyflwu0UjqDObwq0lM3JZXbKxvbbq1Bobc2m9is068yNNoF7L524M6cnnnyWZlX/Lep6Uv5+yZ2aGCdx9+cyq/C4meVS9FvRd2mDsrIi4ppLkVPlp/Wxr/BEJzg19TZbzjr07JVm57lNlkxOvWCtbIrZd5AXVZ8ZkeSBZfpSfE1s56dnLGhlDWJ6dMXqXI63ruKwRDEZoIyOzwmYRkJyRhzwuh1TOLmTKrOi+2irx5Hp0nQR8D1A2RUYPpLhtZPZA42MqNuuNcry/sPfUGvlcbNyrOf3P5FWY03cFpTHl/YryVC7RCcs3IqL7RvnE9N2U5EkaENob89pMDf1a1Yt8/p08NjF7RpUzYrI+sTJMmNyC3k9Y2a7EZjkjorSV/BHrLwm9O9eee1LPkYzf6mRU3/m0PPeW0BzXdYKiofLsU0EOaZ1xK81l4uFCaKljX1ODvE9MXV7+688vPtotDVCuC9UfSJnGFstsRpbBaakQD8XPYIqUEQnvm9LK+K38HJMVUej9hfKR6aqIuy3rgQhrRn8fUlry529ERLf0/HX5vz9IpusGk7UDls+z4fmEEZ4gnTYhK0fmo8pfKHLQ6kfJeUx1cKNbffT8La9Pmbb46ZKcMHnUe0uqA/XcSmwHlT1ZZBRMxgk9M5otUKMlNAoffxipeGkD+adiPOt9fRHaxLHMJyEt5hoM0ddOHN+PAveLTiDCOSXlup7pU2i3yZo/U24ucmG9amVTFsBNA7lVmvcGy4l/f23YNqAijpe07iaY4hc1HiKVjyTSFVliMSOdI6U3iHTlR4bvXzMyjjRy4LJmFW2vvjtnxM/106Mmo+pzj1odqDqaEDnK9FODjFz/xdzyBKHVK5jJSGXLa8hsMkIyfmsww/QBF7ejC7F2dp2RWzsaoE1c84wd+/DF3/71I0jQBinVQ7Fxwc3JhcYJRK0TJerD+2ITC0C5JumnsmiWuuVE38dEOqYJ6VeNDNsgUVYL/b1iFipv55xZns+WvpBraS6r8mT9PtEIuK7P6XV6xP6fWGSMTTKK8BGku01oZN0kIybbuga8GiEZ5z1YaOsB03FLw8SxzlwsjfOB2sSV0I461pMkssPyswxZOGlxlR8Zsv/hgpQWCluPUmvM/DNjclZZAK9a31Xu4nsWjMLfrwtesK0F8U3hJkKNDF6bR61fyTQKsoiOLF6eD3Jg1lraYBKasijL3BCIo4fV257jRHdkqatYI9GDUFb+vllol2LcyHZN/ppTNOIRtF8cuE1sRBqNfPLkq85k+5+WJHYcmsgMxLZkyjlhcsv/n5o+5LprYwEo8rizWE5xTVo2S7BOBt2yUvnJvyO2Jy2zPa8TlNiOUKxr+7yG+D/C6tlEIrekV9RELupkHI2N0EYXtk+RX2NXMFmF/LNdss5IofuWdx3B1/JILBdCKxz2dI198uRq4UliuC9JLB1Y1glT0kru7zXh8FUWgOrTaiOzTHNpCJVPtfyrrKyigihte04TzWpRkK5LGaQx18qgLMG8ghijBkT+kY5NfuaGVfUw1aw19VxsIt0KKy7XJgw5LDQ7piNXInXK8+sIZc4clWIfRHEQqHx1mO0ik/3155e6SZ+0fqRb8WxoMqNw8pgpVtVedUExUQNCi1lfW1Qo30yzMrgF+ZEWj8ikNT+ZTl4Rlm5zIa40Ob5zOfg2AKofTg5Kv5ybtj0wGVO9LrQ1xVsi8pTStW2C1gktt8j44To1yAALzYKTkeuSugac9phXIwIWmzWMqhmmb7TdaO1ioXU6dseDNa3CtOcD9MPI0tZLssh6nzWT0rtlfXeitesFKVapENfBB+XPd03Byv54SpZblQVQsImTik7MKyynlPKZanlGFI4uXZVSfhmRmYrNIeEyn7O2pEtKPzOQxTP974l9P+HjnULi1XdSHl3WrTwpL1Vf71q6fPzbXIk66T5SnzbJWOc6BaF5VGy8s8tw01WVNWU4mSLRLIaINV4kNmfT2dLzQWayw1z3eD3MQcC0JWHcNHzWhVi7DirXYJAF9bXOhCb7YMe2PtL6+/rg4yZRi+qGhzLfG8/tnTMiS1h/lp97zco5FZuzWBOxOYl/xdoztU1WSOkrElqJzX4xXZYVPS83Wp+JzcbrhPJTJHJdfv+D+kRC78t3FzUuNSn3ynIqykJZj2xrgtxDJlg+KbXdpVZWVT9TJo+qv4mhblTZplrZFky2TFhuA9DqSsr4haWVUlnmYjvyMRUjOcNRYXTXx5SD7d8ekjke4r4zl5PiGQ5dT7xoKfM3EW7zd0HlKWpkkJOFd4d8TtuSgoc85XU8h0QOL33KLvHXn19eaJIlleGyIZHNmCKVk70z8clArrJbUuyfrvz7jlFZaJ5ObE8HvLzT1V266pPMPFrENqijh5Y1z0UumXS0cFyDQVYDD5dF07UxIjL91oA38clAZPbCLCAAhAYEJIdezXfP15jYMG9AaC4Tma6TF1f38OOQHa2OzNh9gZeWCUP2mQYmIzM5YTwbU2Qe4A+/72GZEpqR9m1dRsI9/L1vJRP1kUcDy/vIIf28Q1tNHcueMS/AqBSjJDJyfUt36m1FOT+bQlfBHacgM1hou4aHclBLJbnweTtwD+TQN6H1dXCyutE2RN11cZ25BoN8WGfSRUw3mneFlz2ANKFSQQG1aQ7olh8EKroSKh8WWp/w2eHWNw73aK0lrmXvkXx9WEZtreYoELG2Uswkh6u7cTmWSQW7wV1aZLOGBPmpyAwAoQ2CAAERE7LWXnpYL3INfx9i9tjn1TZzi0KOe6431wnOaoCJh6ne5CThiRHZmD0BAPApLTSJNJD19E1e4Oh4X1cVIsf3h9jPEfWYl+2uNCcZOrjOvLkbByQzdZBvV0vzuwBGA3mqB902EKE23DDGNbRnEe70iiuxOcHiLgBpuuC8lMvnKSmvVRtnPW2R6FL/Nx6txFaTHw/BIPLYs0HD9T0dppz2JW+ppOW+0oIukfSi/MXPyydlHawDPWrOgxwjgSU0IVnRXjh1kfFCND+IANgRQuM3x4bA+oyzUjHImbqXEzk8uTMjzxaTdEktK9y4Q8wGzw0D1oXE856ts9UIxoePvpb3ISg7C9Cni3POyEydSLJThCZ+PcFfjcVCAE4YncuRFHDak2KQa2sPNQELTclojKgaIEcDyBMZgnRc6q6x68xTMMh9gDr50jOhFT1u3ldt68Wdrl3ieeFhQjQU9FsEluLnRu8lKGn/LDRBpnfSU15ygEwd3ZDxCOuwLmpyKJkvtYHrQmhpy3Z2rc8QwRRt28F1IhIsIIQ2L08Nijs3WG4f1hvdfdYE6uLcpai+xVp5OxI2sVvx/WfsmYxklgf/3jDiVO5p07vqsF51u7Nqw5XpnMTy+anYvijUel0Lk+uXiQhPm2TkeUv5frmNm2RNxPYByLm+F6+qTfht32Jz+s+aiOlg5GldHXxqQpNuwJJg1MGjfUC5IeXVL2cdotiORliNdbPwoQgtVgfyeljHazNwXN2N9yNp16jHOmtjOT1VTEJzplwf9L5X/n/V8FzFj2tiSJH+Uh/aif8ct/JwYqaoXxhhKWV/Q3er3Yrt7Q/y3Qu6qFSIzaHKhfbcnOdB5X2ytRmVQd3aLdvlmCZec0s9H9OhzCYZ1TN3dNCyrU3m9Nz6RoOKNpFXzqjjwdQan6m8lXXQN8Z8UshC9B9eLBv/vcOa2BgtNKubh6INh7yE9NJDvTW+1JOI05UIViNpV9e+FiLCUSlOOV7Xp3EIdlYiKU6lYGOqy1N6VpLdlN3sbCPNmTBfE6O3q3JBXjM5rhkR6e8pWY7J0nhg+udUbG7XuNX0hHr3THturpF8xPLgsqRae6oyLcX27d1nYrN0MCFL6oHqYsGeU2T7o6ZNigZtoq72mWl1VbB6XTG5L7Q6GOzS3NGeFCKVVamIZCW996x81zMmmXcLF1M0wirMR0zAU1rT6utST+dgkAEOjbYRsyu8ThKZS0vWzym7MVq/P+tKKXdujdFzDw3GkLJa+LaJlBTvhF2REjHXl2ozqbxlH4g1l560Qi6YLE/0K4+clO+eUD4JG1db79Jzcza2ZhXl5eNTPf+mrDY+dsvnXxiRp0zG6/LvO/bcJZOjqk0mLH2bjIoQT4go1SThQtUL1aOcADwzy1XVwWD6cNRnOdIM/HAAS209c2myZ22g8HdXxeVKaD6slbno4VBidkivC55H0qaRhzGV+p6c0M977U4w3fL4yiynphMvbp1Fgt2kTOQyMfTnj1uVdYueje1Y92IQCUREVGmDNsg1GXUCV+W9t4w9FeF4ZOvPdGO2srKuWX0XnMwMcqlx9VjTJueWNuF19fGeVi+/1AO7xXqwyd/oz3Jkltpc9LemphrsVlTf9jxW66xuw7HLml9GdZI4Ws6uJPO9p3wKEdbdGAV6tq3V3hU2hZxoilspu3eyANrgnJX/pcZatE12lFLOyFrT60PJm9DeOVv9zSz9LzZZXgZy1OvLRpAPlFemrKyG5cvJolQWrEnG14Zt8maw6hRODHLHIbwAe0VozFK7LontuaF7whfUJux8xwitrkO5yJzRJONeuF9m2ofrzPnes8BHXbVpC9cQ9RCEZlNiH6Hp2n40m7W7tJBUIjbus0dDfajvU5ss5B6bsP+bnjtichgnS0QWRzVkwUkga1BfiSLaKjKrqmtGdKnWp5q0ialeuSfGZnVFFVbbYHft7dRp+xQZd9yztTavsdLGuA+mTnElHiyj5ZCE1sR15ikY5HFE7TrG+/aUYissRJ2x359VaHzL8Sdxr7vZSqV8o/XlyGI9JZqCj3USYe8+dnE5apYXD9E3vV+wEP0tcqwgM573j4q65uUrKqzICSvvTYXVe6X3HSLsqKHVBkIbmbU2rSG0XYtwdK2vlNpBXpuyFO4uvZCWxrmHvOZlOedDN6inyNQQrqB12LYWmMEtj5wp7lZBQPReQnksK8pzpClUfb+War9ng7UmNPKpq2PbGh0nlYkpLcPJKVvkWH5/W0FmVfiqEbmtTbjbNdHqzoYDQ99pY7WB0EZmrU3UvqlASsY3MlF94oDP0+3vx0ponoJBXK3ZMVlnoWbOK6rnl1JJKnfiF83yyEjJzmit5lkrky0QQxHR0qLcdRJSBJEwt5+6sVvmsdJcbRzPNHl9INJTltaElPRNzRrWB4ETOcmfMUUlvlnIIdGI6IrV25W2rnVH/5d1cknp8/LxdFWbPFCbFKwsmSId2kM2JavQ1iYm8opbWG0gtI7W2m1AaykR5lMpfOR32HNIuOver5zVf1bWfTqA0m/iOpuJ/ULsYbyEsNCuSbbYIGNOylO612Rg1xO1y6yh5ThjE6cmBB1pRMi9CmcaiWz1IRl6Tutjpo3ZUs/cMOLU17AmGoELyk/diKAfuaZHOHLijy0T9DsiIeUVeWB1kIvNqSe8TUx5c9mbtEkitFNKLPUweEDIzhMat9bKH8d09fx8x5RM3zMa1whHHfcDEFqTQXO5K/1XuoEb9INRHnlFltMxOw7qIz9uVZFCPGQWkmjgnpIKNze499S70p0pvTSF5s7jYej6+3c2i5BO2Fho4/rjfSKUU70uGWFnWnn/qCmvlPNe/a/8aapHQYSiNkRflM/cM8sxVW5U9oypTc6JtN5atsmx+HUtcKnXoa1u+sZvezaLVSffP3i21mSk25khrwdHSyAt0z3tuX6+OdTNXSnvtSHNd9FvtOdxlbVBfeDbDnXb07ogF7oDzWXiIG9euBB7Cnbk1aJD4MlegyzIbzRGj4c8axEWWge3Cu1bu/XodrKtk7kq8SE6lgvR2/Z+LZgLpJc23hfrrAVcreB9v9TzYMAxNSbykmQ+F5t1SzXm1Wkje10/v+9joeTaGs1GQ5+/t1NKxsO9bbbBsBL93eWU1ZSRnwa+L14HH9Zv+gn0efrZCY3qQB+PazesdlQXLLQdxL0n5fYaSMn0PficCM3mFqON1kvRz97AvOb7qRhf5Omg7daw3nYacv1LAGr9K/2s5f8dXaAznAktwLl6IWWuU4h9Xa9SdwrBProbd+lSTwAAoY14ZmuzpBJXJTNAfbicapLXkLP8ftlDGayTAHKpxnvYT0d7qScAgNB6AK2l+Arh3xeF4GKhNdn71ccxUfkeWmeTgO0GQgM+DQZfQyPieWCDWloub2wgKksma3Gho1xHuRV+1lJyi7vG9QxHeQLJtEx71WM9uyjGWqVIp7ekIuC+NJvrbB+DQTx6GrwHH7ErXe5aHNFUld76GDt2S/IgoOOnZH1fN40IpP1e8r1XbBn45IRGHUFXRFOL0uKz9Lxi8PsMCkgDll3eucZJ29Vyua4gfZ9HXtVZaaEIraot9jEYZJSXerJxK+v7zlN6s4AkJevQukHb0I+ilmMyoc+rAD49oXVRtJHobyPv/QjLb8NbhYJxjXDMGj63pMN8Q7RPlQw+3I0Xols0YMhj15zr0XfwETsnsfBkncWhJo98w7X4eXxVkz6WNiQ/hYOWkz4AhDYI0gpFnonxHFyrUDQYdH3M8O/Fr+fh+cB3ixXjIxhEupaXHa2osdyX5mJZdxmzmecyhrAkj9qQZUlkZyOoD2AXCc3DRt/QWFR892OE8maBJg5tlaIkhrnw7wK0lc/HNTH3A7ZbVRTjmC/1fCUrSLrpnog0JmL7PjC5FrVkFtOU+kZsm7CUz6yPaSrf+0OzttRRc6d0duANtb1O+teG/vdCp9cvSC454VqJzc3rC6ormcfH8Vm0Vqh7HOT7GVvvM92vBgyAoaMcx0xoqxpXTTo2gWvcgi51/dZSDtv9VaEIbeajvQdsuknAMRJiXedII0slo/JY3FF9rgO+1H1ojPhiGj8L6ieF1r5xjSWXM6KZUBoL+qQk10Jsu99T9lHyTylPJUfEy0WE+aDlsaS/E3qm6nZq4DNZaKLfA23bzmovapR2Sq6msQQiVB3WGznK2YW8pcXj8+SQwhTwUpZt5qENVmPceDziSz0jjdAUQVxo1piyqCJ69tbynLKSsqZ3jpXPfWVpGScjRKSyD6Y8epIiGSW2Duot/3/CCDMiwizIIsxYmjM2JuKAljCwYxbayQjrRHbgs4ZbBO5HJHcWcOLQerASQax6KJ8Pd+PzSMfnWI+8UsSiK/WVzfIkopL9MONkxvpnTgEmRkJjt16nWrnm7D4yW/3lhv8XBhdhzMqlIq31A331NJMuXgxgPwltbBbaejbW4iLEOzHMiR8mfK9TQF3hYL34JHzbeZqJa5t3DQbZBULzfaknI5ZMG8emiMcJU/5T0+TBcIN0ZOnPOpGoNS/5/3dyZ9rq700jVhNhKms41ybbz5Y0lXwHDl4MAIQWDHlLMvu4MXsk8lcNqINA6dbVTyr8ubxMpOojVH854vE5xiOvtohFu1jTZsnlFYo/1iYsJ5b0tshJXWIpNmt1T+TiNNVfZtA5tvQzy996mqmWXi6Az0tonjaM+oIcFMddZrM0ux+DUgwVEOKqFH1ZaSaFMfOQro/julwDL+qCIMZIaG9VBGHYW2ZT/AcmC81wo/Qv5CRJjcLsz8hTMqNgEVFBSnGNBfhWY3Xq5eWkDXxyC20MVplcL2u6ZmYjtQtRHeIfGkWN/CEu9WxD+M6DXY849RQMkvl2yXWErRyuk74Q9+3pxBJpRFBnCRVVFl8FidvSExQUcmF430RKNgvQdEFobhlL6zRDbggHdo/QCjHc+pPsqBelMjv0dZZimc5N+eN0INdDVYRjqEs9+7SCTG30dSTWWSgPRuQhmRCKVre0TizKX7eE1Pe3WhBHrFlkBVl4ibL0KHx+i0h4GvT7uel9Q/+1EaNOrOsAFbU2J9MrPy8WMod1NhIMFrYvZ8bloJU+8CkNikSEDYHPaYDfh5qVkxVxSNbDpehvn10oxVx4Ojbpjggo7thupnVK14OIQ+2V6ypLU6ut78mI0dpnkX86IdksOXXZrhwbM9rkbJJTPifD5V+0Z/hzD0Q2pu+XGslIIvo3keEfYjui8hdrmJVLyfHE8rHtl0OE42cnNCKAnJTdHbMmVHjvidg+daDLYM6os6V9upbUuhrNsn0Sds4Gqlq3WVWVjSYO1x0smlx4Wv+iG62l9Xol2m3VkBFmS4s79UJ0D9nPaWLjy0Mg+++Xjn1VyrCwtNtCdNvaIsv36LF8HAvNIllUTLJelfKXRFESwzER2onWhz/IUJ7QUT6Xs/76St9PGZEsqK6PxGZTsxznK0VUtFftVGxOEnlm72aWchUN5Vhpcg+5KR9g+G1XBG0RRJIFGsh9l2MnygMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACEwP8LMABlxjRRiM1J1QAAAABJRU5ErkJggg==";

// ─── Página HTML embutida ────────────────────
const char PAGINA_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Sistema IoT — ESP8266</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Exo+2:wght@300;600;800&display=swap');

    :root {
      --bg:      #0a0e1a;
      --surface: #111827;
      --border:  #1e2d45;
      --accent:  #00f5c4;
      --danger:  #ff4d6d;
      --text:    #cdd9f0;
      --muted:   #4a5a7a;
      --mono:    'Share Tech Mono', monospace;
      --sans:    'Exo 2', sans-serif;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      background: var(--bg);
      color: var(--text);
      font-family: var(--sans);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 2rem 1rem;
      gap: 0;
    }

    body::before {
      content: '';
      position: fixed;
      inset: 0;
      background-image:
        linear-gradient(var(--border) 1px, transparent 1px),
        linear-gradient(90deg, var(--border) 1px, transparent 1px);
      background-size: 40px 40px;
      opacity: 0.35;
      pointer-events: none;
    }

    header {
      text-align: center;
      margin-bottom: 2rem;
      position: relative;
      z-index: 1;
    }

    .logo-senai {
      height: 48px;
      width: auto;
      margin-bottom: 1rem;
      filter: drop-shadow(0 0 12px rgba(0,245,196,0.15));
    }

    .badge {
      font-family: var(--mono);
      font-size: 0.7rem;
      color: var(--accent);
      letter-spacing: 0.2em;
      text-transform: uppercase;
      border: 1px solid var(--accent);
      display: inline-block;
      padding: 0.2rem 0.7rem;
      border-radius: 2px;
      margin-bottom: 0.8rem;
    }

    h1 {
      font-size: 2rem;
      font-weight: 800;
      line-height: 1.1;
      letter-spacing: -0.02em;
      color: #fff;
    }

    h1 span { color: var(--accent); }

    .subtitle {
      margin-top: 0.4rem;
      font-size: 0.85rem;
      color: var(--muted);
      font-weight: 300;
    }

    /* grade de cards dos relés */
    .reles-grid {
      display: flex;
      flex-direction: column;
      gap: 1rem;
      width: 100%;
      max-width: 420px;
      position: relative;
      z-index: 1;
    }

    .card-rele {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 1.4rem 1.6rem;
      display: grid;
      grid-template-columns: 1fr auto;
      grid-template-rows: auto auto;
      gap: 0.4rem 1rem;
      align-items: center;
    }

    /* coluna esquerda */
    .rele-info { display: flex; flex-direction: column; gap: 0.25rem; }

    .rele-numero {
      font-family: var(--mono);
      font-size: 0.65rem;
      color: var(--muted);
      letter-spacing: 0.18em;
      text-transform: uppercase;
    }

    .rele-nome {
      font-size: 1.05rem;
      font-weight: 600;
      color: #fff;
    }

    .rele-pino {
      font-family: var(--mono);
      font-size: 0.72rem;
      color: var(--muted);
    }

    /* indicador dot + label */
    .status-row {
      display: flex;
      align-items: center;
      gap: 0.5rem;
      margin-top: 0.6rem;
    }

    .dot {
      width: 9px;
      height: 9px;
      border-radius: 50%;
      flex-shrink: 0;
      transition: background 0.4s, box-shadow 0.4s;
    }

    .dot.on {
      background: var(--accent);
      box-shadow: 0 0 7px var(--accent), 0 0 18px var(--accent);
    }

    .dot.off {
      background: var(--danger);
      box-shadow: 0 0 6px var(--danger);
    }

    .status-label {
      font-family: var(--mono);
      font-size: 0.72rem;
      letter-spacing: 0.1em;
    }

    /* botão toggle */
    .btn-toggle {
      grid-row: 1 / 3;
      grid-column: 2;
      width: 110px;
      padding: 0.75rem 0.5rem;
      border: none;
      border-radius: 8px;
      font-family: var(--sans);
      font-size: 0.82rem;
      font-weight: 600;
      letter-spacing: 0.04em;
      cursor: pointer;
      transition: transform 0.1s, box-shadow 0.3s, background 0.3s;
      position: relative;
      overflow: hidden;
      align-self: center;
    }

    .btn-toggle.ligar {
      background: var(--accent);
      color: #0a0e1a;
      box-shadow: 0 0 18px rgba(0,245,196,0.3);
    }

    .btn-toggle.desligar {
      background: var(--danger);
      color: #fff;
      box-shadow: 0 0 18px rgba(255,77,109,0.3);
    }

    .btn-toggle:active { transform: scale(0.95); }

    .btn-toggle::after {
      content: '';
      position: absolute;
      inset: 0;
      background: rgba(255,255,255,0.12);
      opacity: 0;
      transition: opacity 0.15s;
    }
    .btn-toggle:active::after { opacity: 1; }
    .btn-toggle:disabled { opacity: 0.5; cursor: not-allowed; }

    /* rodapé com info do sistema */
    .sys-bar {
      display: flex;
      justify-content: center;
      gap: 1.5rem;
      margin-top: 1.5rem;
      position: relative;
      z-index: 1;
    }

    .sys-item {
      text-align: center;
    }

    .sys-item .key {
      font-family: var(--mono);
      font-size: 0.6rem;
      color: var(--muted);
      letter-spacing: 0.14em;
      text-transform: uppercase;
    }

    .sys-item .val {
      font-family: var(--mono);
      font-size: 0.8rem;
      color: var(--text);
      margin-top: 0.15rem;
    }

    footer {
      margin-top: 1.5rem;
      font-size: 0.72rem;
      color: var(--muted);
      text-align: center;
      position: relative;
      z-index: 1;
    }
  </style>
</head>
<body>

  <header>
    <img src="LOGO_SENAI_BASE64" alt="Logo SENAI" class="logo-senai">
    <div class="badge">ESP8266 · IoT · 3 Relés</div>
    <h1>Painel de <span>Controle</span></h1>
    <p class="subtitle">Técnico em Automação Industrial</p>
    <p class="subtitle">SENAI - Celso Charuri</p>
  </header>

  <div class="reles-grid">
    CARDS_RELES
  </div>

  <div class="sys-bar">
    <div class="sys-item">
      <div class="key">IP</div>
      <div class="val">IP_ATUAL</div>
    </div>
    <div class="sys-item">
      <div class="key">Protocolo</div>
      <div class="val">HTTP/1.1</div>
    </div>
    <div class="sys-item">
      <div class="key">Placa</div>
      <div class="val">ESP8266</div>
    </div>
  </div>

  <footer>Cada botão alterna independentemente o estado do relé</footer>

  <script>
    async function toggleRele(id) {
      const btn = document.getElementById('btn' + id);
      btn.disabled = true;
      btn.textContent = '...';

      try {
        const resp = await fetch('/toggle/' + id);
        const data = await resp.json();
        atualizarCard(id, data.estado === 'ON');
      } catch(e) {
        btn.textContent = 'Erro';
      } finally {
        btn.disabled = false;
      }
    }

    function atualizarCard(id, ligado) {
      const dot   = document.getElementById('dot'    + id);
      const label = document.getElementById('label'  + id);
      const btn   = document.getElementById('btn'    + id);

      if (ligado) {
        dot.className      = 'dot on';
        label.textContent  = 'LIGADO';
        btn.className      = 'btn-toggle desligar';
        btn.textContent    = 'Desligar';
      } else {
        dot.className      = 'dot off';
        label.textContent  = 'DESLIGADO';
        btn.className      = 'btn-toggle ligar';
        btn.textContent    = 'Ligar';
      }
    }
  </script>

</body>
</html>
)rawhtml";

// ─── Gera o bloco HTML de um card de relé ──────
String cardRele(int id) {
  // id começa em 0 internamente, exibe como 1/2/3
  int num    = id + 1;
  bool ligado = estadoRele[id];

  // pinos legíveis
  String pinoStr;
  switch(id) {
    case 0: pinoStr = "D1 / GPIO5"; break;
    case 1: pinoStr = "D2 / GPIO4"; break;
    case 2: pinoStr = "D3 / GPIO0"; break;
  }

  String card = "<div class=\"card-rele\">";
  card += "<div class=\"rele-info\">";
  card += "<span class=\"rele-numero\">Relé 0" + String(num) + "</span>";
  card += "<span class=\"rele-nome\">Relé " + String(num) + "</span>";
  card += "<span class=\"rele-pino\">" + pinoStr + "</span>";
  card += "<div class=\"status-row\">";
  card += "<div class=\"dot " + String(ligado ? "on" : "off") + "\" id=\"dot" + String(id) + "\"></div>";
  card += "<span class=\"status-label\" id=\"label" + String(id) + "\">" + String(ligado ? "LIGADO" : "DESLIGADO") + "</span>";
  card += "</div></div>"; // fecha rele-info + status-row

  String btnClass = ligado ? "desligar" : "ligar";
  String btnText  = ligado ? "Desligar" : "Ligar";
  card += "<button class=\"btn-toggle " + btnClass + "\" id=\"btn" + String(id) + "\" onclick=\"toggleRele(" + String(id) + ")\">" + btnText + "</button>";
  card += "</div>";

  return card;
}

// ─── Monta HTML completo ───────────────────────
String montarHTML() {
  String html  = FPSTR(PAGINA_HTML);
  String cards = "";
  for (int i = 0; i < 3; i++) cards += cardRele(i);
  html.replace("CARDS_RELES", cards);
  html.replace("IP_ATUAL", WiFi.localIP().toString());
  html.replace("LOGO_SENAI_BASE64", FPSTR(LOGO_SENAI_B64));
  return html;
}

// ─── Aplica estado físico no pino ──────────────
void aplicarRele(int id) {
  bool on = estadoRele[id];
  digitalWrite(PINOS_RELE[id], (RELE_ATIVO_LOW ? !on : on) ? HIGH : LOW);
}

// ─── Rotas HTTP ───────────────────────────────
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", montarHTML());
}

void handleToggle() {
  // URI: /toggle/0  /toggle/1  /toggle/2
  String uri = server.uri();          // ex: "/toggle/1"
  int id = uri.charAt(uri.length()-1) - '0';  // extrai último char

  if (id < 0 || id > 2) {
    server.send(400, "application/json", "{\"erro\":\"id invalido\"}");
    return;
  }

  estadoRele[id] = !estadoRele[id];
  aplicarRele(id);

  String json = "{\"id\":" + String(id) + ",\"estado\":\"" + (estadoRele[id] ? "ON" : "OFF") + "\"}";
  server.send(200, "application/json", json);

  Serial.printf("Relé %d: %s\n", id + 1, estadoRele[id] ? "LIGADO" : "DESLIGADO");
}

// ──────────────────────── Setup ────────────────────────────────────

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ESP8266 — Painel 3 Relés ===");

  for (int i = 0; i < 3; i++) {
    pinMode(PINOS_RELE[i], OUTPUT);
    estadoRele[i] = false;
    aplicarRele(i);
  }

  Serial.print("Conectando em: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("Acesse no navegador: http://");
  Serial.println(WiFi.localIP());

  server.on("/",         handleRoot);
  server.on("/toggle/0", handleToggle);
  server.on("/toggle/1", handleToggle);
  server.on("/toggle/2", handleToggle);
  server.begin();

  Serial.println("Servidor HTTP iniciado na porta 80.");
}

// ──────────────────────── Loop ─────────────────────────────────────

void loop() {
  server.handleClient();
}
