# Cpp_MultiServer
1.项目仅供学习参考
2.项目里Copy了如下大佬的内容:
    2.2 mkulke&PhilipDeegan&wadealer&mkay229几位大佬通力合作的libftp(https://github.com/mkulke/ftplibpp)
阅读这个项目的README了解到这个比较nice的ftp也是在Thomas Pfau大佬(http://nbpfaus.net/%7Epfau/ftplib/)的基础上逐渐优化而来,后续我会持续学习并且在此基础上继续改造这个ftplib.

3.这个项目基于C++17,在目前已有的这些工具类中,以高内聚&&低耦合为终极目的,除了FTP类尚未改造,其余尽可能贯彻了单例模式以及pimpl惯用法和RAII的理念.
4.此外,这个版本的数据库工具类也尚未开发完毕,因为并不知晓在各种领域里,数据库连接模块设计理论上的异同,后续会持续学习并填充到项目中.
5.其中libftp(https://github.com/mkulke/ftplibpp)项目使用了开源协议LGPL2.1,如有Copy或者商用需求,请联系libftp项目作者