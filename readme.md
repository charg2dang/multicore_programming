

Lock benchmark


|                           |1thread|2thread|4thread|8thread|16thread|
|---------------------------|-------|-------|-------|-------|--------|
|TAS_LOCK                   |1672   |3734   |8485   |17719  |31266   |
|TTAS_LOCK                  |1609   |3719   |6016   |8140   |27703   |
|Backoff_TTAS_LOCK          |       |       |       |       |        |
|A_LOCK                     |2406   |7422   |8156   |8391   |7953    |
|CLH_LOCK                   |2344   |4344   |7750   |9266   |11938   |
|MCS_LOCK                   |2328   |4062   |8344   |9506   |9297    |
|std::mutex                 |5610   |6828   |11625  |21781  |26125   |
|CRITICAL_SECTION           |516    |828    |1437   |5844   |6500    |
|SRWLock (Exclusice Mode)   |484    |906    |2594   |5047   |6484    |

                                                                    단위 (ms)

(benchmark/KakaoTalk_20201118_174752756.png)
