DEB패키지 만들기

dpkg -b llpr_ex_hipass  : llpr_ex_hipass.deb 패키지 파일 생성됨.

DEBIAN/control  : 패키지설치 시 패키지명 등의 정보가 있은 파일
그외 디렉토리 : 패키지설치 시 /에서 해당 경로에 파일을 wirte함.

deb파일은  확장자를 deb에서 zip로 변경한다.( 원격관제에서 사용할 경우임. 그래도 설치가 됨. )
예) JWIN_20200206_IPUH_F10.deb => JWIN_20200206_IPUH_F10.zip  => 다시 zip으로 압축함. JWIN_IPUH_F10_1_2_5_18.zip ( 최상위 zip의 파일명 형식은 없음. '.'만 사용하지 안함. )

JWIN_IPUH_F10_1_2_5_18.zip  : 모든 라이브러리 포함.
JWIN_IPUH_F10_1_2_5_18_fw.zip : 실행 파일만 포함.

dpkg --force-overwrite -i JWIN_20200206_IPUH_F10.zip  : 패키지 강제로 설치
