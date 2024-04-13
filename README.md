# Zkvstore
简单的kv存储数据库。

## 环境
> Linux  6.5.0-27-generic #28-Ubuntu SMP PREEMPT_DYNAMIC x86_64 x86_64 x86_64 GNU/Linux

## 构建zkvstore服务器
```
./server 
```
端口默认为8888

## 可用命令
### STRING类型
- #### `SET key value `
    设置key及对应value，如果key已存在则覆盖

- #### `GET key`
    获取对应key的value
- #### `DEL key`
    删除对应key
- #### `INC key`
    使对应key的value+1.如果value不能被表示为数字则返回ERROR
- #### `DEC key`
    与INC相反


### 有序SET类型（红黑树）
- #### `RSET key score1 member1 [[score2,member2]...]`
    将多对{score,member}加入rset中
- #### `RGET key [begin] [end]`
    取出排名在第begin位至end位之间的所有key
     
    若end所在的索引在begin之后，则是从小到大返回；反之从大到小返回

    可以为负数，-n代表倒数第n个

    可以缺省，缺省值为begin=0,end=-1,即升序返回全部
- #### `RINC key member`
    使得对应member的score+1
- #### `RDEC key member`
    使得对应member的score-1

- #### `RINCBY key member num`
    使得对应member的score+num
- #### `RDECBY key member num`
    使得对应member的score-num
- #### `RSCORE key member`
    返回对应member的score
- #### `RANK key member [ifback]`
    返回对应member的排名

    ifback == 1 ==> 降序

    ifback == 0 ==> 升序

    缺省值为0

### 哈希类型
- #### `HSET key field value [[field2 value2]...]`
    设置多组{field,value}

- #### `HGET key field [field2...]`
    获得多组field对应的value

- #### `HDEL key field`
    删除对应field

- #### `HGETALL key`
    返回所有{field,value}

# 协议
- # 每句命令必须以'\n'结尾
- # 每个命令数必须相隔一个空格

### PS: debug.hpp来自于b站up主：双笙子佯谬
