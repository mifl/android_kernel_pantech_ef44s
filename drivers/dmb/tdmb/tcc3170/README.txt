
 1. director dtructure
 sample driver
│  tcbd.h
│  tcbd.c
│  Makefile
│  
 SDK
├─src
│  │  tcbd_drv_io.c
│  │  tcbd_drv_rf.c
│  │  tcbd_drv_ip.c
│  │  tcbd_hal.c
│  │  tcbd_drv_peri.c
│  │  tcbd_drv_dual.c
│  │  tcbd_api_common.c
│  │  Makefile
│  │  
│  ├─tcpal_linux
│  │      tcpal_linux.c
│  │      tcpal_io_i2c.c
│  │      tcpal_queue.c
│  │      tcpal_io_cspi.c
│  │      tcpal_debug.c
│  │      tcpal_irq_handler.c
│  │      Makefile
│  │      
│  ├─tcbd_stream_parser
│  │      tcbd_stream_parser.c
│  │      Makefile
│  │      
│  ├─tcbd_test
│  │      tcbd_test_io.c
│  │      Makefile
│  │      
│  └─tcbd_diagnosis
│          tcbd_diagnosis.c
│          Makefile
│          
└─inc
    │  tcbd_drv_register.h
    │  tcbd_api_common.h
    │  tcbd_drv_io.h
    │  tcbd_drv_rf.h
    │  tcbd_drv_ip.h
    │  tcbd_hal.h
    │  tcbd_error.h
    │  tcbd_feature.h
    │  TCC317X_BOOT_TDMB.h
    │  
    ├─tcbd_stream_parser
    │      tcbd_stream_parser.h
    │      
    ├─tcpal
    │      tcpal_debug.h
    │      tcpal_os.h
    │      tcpal_queue.h
    │      tcpal_types.h
    │      
    └─tcbd_diagnosis
            tcbd_diagnosis.h
 
 2. porting guide
 tcbd.c 파일은 SDK를 이용하여 만들어진 예제 driver입니다. 해당 파일을 참고 하시
 면 쉽게 포팅할 수 있을 것입니다. 포팅시 수정되어야할부분은 다음과 같습니다. 


 - inc/tcbd_feature.h
 	SDK에 전반적인 설정을 할수있도록 각종 feature가 정의되어 있습니다.

 - tcbd_hal.c  
 	CPU의 GPIO설정이 위치한 파일입니다. 하드웨어 구성에 맞도록 수정 되어야
	합니다. 현재는 telechips EVB환경에 맞도록 되어 있습니다. __USE_TC_CPU__
	로 검색하면 해당 부분을 확인 할 수 있습니다.

 - tcpal_linux/tcpal_io_cspi.c 
	SPI I/O관련 함수가 위치한 파일입니다. 현재 텔레칩스 BSP에서 동작하도록
	작성되어 있습니다.(__USE_TC_CPU__ 로 검색) 대부분 kernel API를 사용하였
	으므로 크게 수정할 것은 없을 것입니다. __USE_TC_CPU__부분만 제거 하시고,
	spi prove 함수에서 spi_setup함수를 호출하면 될것입니다. 또한 driver init
	함수 또는 prove함수에서 tcpal_set_cspi_io_function를 호출해주어야 합니다
	. 해당 함수에서 관련된 함수 포인터가 초기화 되므로 해당 함수를 호출해
	주지 않으면 driver가 정상적으로 동작하지 않을 것입니다.

 - tcpal_linux/tcpal_io_i2c.c 
 	I2C I/O관련 함수들이 위치한 파일입니다. tcpal_io_cspi.c와 같이 kernel
	API로만 작성되어 있으므로 특별히 수정할 것은 없을 것입니다. driver init
	함수에서 tcpal_set_i2c_io_function을 호출해주어야 합니다.

 - tcpal_linux/tcpal_irq_handler.c 
	인터럽트처리 관련 함수가 위치한 파일입니다. 현재 예제 드라이버에 맞도록
	되어 있습니다. I2C/STS를 사용하는 경우와 SPI만 사용하는 경우가 다르게 
	동작하도록 되어 있으며 __CSPI_ONLY__, __I2C_STS__ feature를 통하여 선택
	할 수 있습니다. 

	__CSPI_ONLY__경우에는 MSC data에 4byte 헤더를 CIF마다 붙여서 전달합니다.
	SDK에서는 그것을 파싱하여서 callback함수로 전달하도록 작성되어 있습니다.
	따라서 tcpal_irq_stream_callback함수에서 파싱되어 나오는 스트림을 적절히
	버퍼링하여 사용하면 됩니다. __MERGE_EACH_TYPEOF_STREAM__를 적용하면 인터
	럽트 한번에 포함된 여러개의 CIF를 머지하여 callback함수가 한번만 호출됩
	니다. __MERGE_EACH_TYPEOF_STREAM__를 적용하지 않으면 CIF마다 callback함
	수가 호출됩니다. 모니터링 데이터는 스트림에 포함하거나 레지스터에서 읽을
	수 있으며 기본적으로 스트림에 포함되도록 설정 되어 있습니다. 예제코드를 
	참조하십시오.

	__I2C_STS__의 경우 인터럽트는 FIC데이터를 읽는 경우에만 사용되며, MSC데
	이터는 TSIF를 통하여 188 bytes단위로 전송됩니다. 모니터링 데이터는 spi와
	달리 스트림에 포함되어 있지 않으며, API를 사용하여 register를 읽어야 합
	니다. 예제 코드를 참조하십시오.

	스켄시(주파수만 설정하였을 경우) FIC data만 나오며, 서비스를 선택하면
	FIC는 더이상 나오지 않고 MSC data만 나오게됩니다. 서비스 선택시 FIC data
	의 유무는 feature(__ALWAYS_FIC_ON__)설정으로 on/off할 수 있습니다. 

	driver init함수 또는 prove함수에서 tcpal_irq_register_handler함수를 반드
	시 호출해 주어야 interrupt handler가 정상적으로 등록됩니다. 그리고
	tcpal_irq_register_handler에서 tcbd_irq_handler_data.tcbd_irq부분을
	해당 H/W에 맞게 수정해야 합니다.
