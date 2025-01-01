<script type="text/javascript" src="//cdn.plu.mx/widget-popup.js"></script>
# IrisExamples
IrisExamples provides example implementations of the [Iris Modules' Application Programming Interface(API)](https://ryanlandvater.github.io). 
This is a part of the Iris whole slide imaging (WSI) platform written and maintained by the authors of Iris, specifically Ryan Lanvater (me). 

There are three (3) requirements for the successful implementation of Iris:

1. Add the Iris modules headers to your applications search path for reference during compile time. *These headers are included, for reference, in the example folder for each module and generally comprise both a **types.hpp**, containing the type definitions and API call info_structures, and the **module.hpp** header, which contain the methods required to implement the API.* 
2. Link the Iris modules libraries to your application during program linking. *In order to gain access to the Iris runtime binaries, you must agree to the academic use license. You must contact the authors of Iris for access.*
3. Bind a drawable surface at runtime to allow Iris to configure graphics parameters. 

Iris Modules with examples will be updated as more modules pass code review, manuscripts are published, and are as follows:

 - **Iris Core** : a simple introduction to the basics of binding an Iris View and slide rendering surface to your whole slide viewer application
	 - [iOS implementation](./IrisCore/iOS/)
	 - [macOS implementation](./IrisCore/macOS/)
	 - [Windows implementation](./IrisCore/Windows/)
	 
	Iris Core is called from within the Iris:: namespace. Iris Core is implemented by constructing an **Iris::IrisViewer** ([IrisCore.hpp](IrisCore/IrisCore.hpp)) instance. An Iris Viewer is created by calling the **Iris::create_viewer(*create_viewer_info&*)** method ([IrisCore.hpp](IrisCore/IrisCore.hpp)) in an inactive state. The viewer is initalized once bound to a drawable surface, such as an operating system window, via **Iris::viewer_bind_external_surface(*bind_external_surface_info&*)** method ([IrisCore.hpp](IrisCore/IrisCore.hpp)). Calls to interface with the engine are made as part of the remaining API methods defined in [IrisCore.hpp](IrisCore/IrisCore.hpp), such as **viewer_engine_translate** or **viewer_engine_zoom** to control the scope view.

## Publications
For publications related to Iris and explaining the functionality / architectures of the various WSI modules, please reference the following patents and publications:

Landvater, R. E. (2023).  _Patent No. US-20230334621-A1_. Retrieved from https://patents.google.com/patent/US20230334621A1

Landvater, R., & Balis, U. (2024). As Fast as Glass: A Next Generation Digital Pathology Rendering Engine. United States and Canadian Academy of Pathology 113th Annual Meeting Abstracts.  _Laboratory Investigation_,  _104_(3), 101595. https://doi.org/10.1016/j.labinv.2024.101595

<a href="https://plu.mx/plum/a/?doi=10.1016%2Fj.jpi.2024.100414" data-popup="right" data-size="large" class="plumx-plum-print-popup" data-site="plum" data-hide-when-empty="true">Iris: A Next Generation Digital Pathology Rendering Engine</a><l>
Landvater, R. E., & Balis, U. (2025). Iris: A Next Generation Digital Pathology Rendering Engine. _Journal of Pathology Informatics_, _16_, 100414. https://doi.org/10.1016/j.jpi.2024.100414
