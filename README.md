[![DDS Enabler](resources/images/github_banner_ddsenabler.png)](https://eprosima.com/middleware/tools/fiware-dds-enabler)

<br>

<div class="menu" align="center">
  <strong>
    <a href="https://eprosima.com/index.php/downloads-all">Download</a>
    <span>&nbsp;&nbsp;•&nbsp;&nbsp;</span>
    <a href="https://dds-enabler.readthedocs.io/en/latest/">Docs</a>
    <span>&nbsp;&nbsp;•&nbsp;&nbsp;</span>
    <a href="https://eprosima.com/index.php/company-all/news">News</a>
    <span>&nbsp;&nbsp;•&nbsp;&nbsp;</span>
    <a href="https://x.com/EProsima">X</a>
    <span>&nbsp;&nbsp;•&nbsp;&nbsp;</span>
    <a href="mailto:info@eprosima.com">Contact Us</a>
  </strong>
</div>

<br><br>

<div class="badges" align="center">
  <a href="https://opensource.org/licenses/Apache-2.0"><img alt="License" src="https://img.shields.io/github/license/eProsima/FIWARE-DDS-Enabler.svg"/></a>
  <a href="https://github.com/eProsima/FIWARE-DDS-Enabler/releases"><img alt="Releases" src="https://img.shields.io/github/v/release/eProsima/FIWARE-DDS-Enabler?sort=semver"/></a>
  <a href="https://github.com/eProsima/FIWARE-DDS-Enabler/issues"><img alt="Issues" src="https://img.shields.io/github/issues/eProsima/FIWARE-DDS-Enabler.svg"/></a>
  <a href="https://github.com/eProsima/FIWARE-DDS-Enabler/network/members"><img alt="Forks" src="https://img.shields.io/github/forks/eProsima/FIWARE-DDS-Enabler.svg"/></a>
  <a href="https://github.com/eProsima/FIWARE-DDS-Enabler/stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/eProsima/FIWARE-DDS-Enabler.svg"/></a>
  <br>
  <a href="https://dds-enabler.readthedocs.io"><img alt="Documentation badge" src="https://img.shields.io/readthedocs/dds-enabler.svg"/></a>
  <a href="https://github.com/eProsima/FIWARE-DDS-Enabler/actions/workflows/nightly-windows-ci.yml"><img alt="Windows CI" src="https://img.shields.io/github/actions/workflow/status/eProsima/FIWARE-DDS-Enabler/nightly-windows-ci.yml?label=Windows%20CI"></a>
  <a href="https://github.com/eProsima/FIWARE-DDS-Enabler/actions/workflows/nightly-ubuntu-ci.yml"><img alt="Ubuntu CI" src="https://img.shields.io/github/actions/workflow/status/eProsima/FIWARE-DDS-Enabler/nightly-ubuntu-ci.yml?label=Ubuntu%20CI"></a>
</div>

<br><br>

*eProsima DDS Enabler* is a modular middleware solution that bridges DDS networks and FIWARE NGSI-LD Context Brokers, providing real-time, bidirectional interoperability. It manages all required DDS participants, auto-discovers topics and types, and seamlessly routes DDS samples into NGSI-LD entities and context updates back into DDS topics.

**Key features**
- **Unified DDS Participant Management**
  Auto-create and discover DomainParticipants, Publishers, Subscribers, Topics and Types without manual code.
- **Flexible YAML Configuration**
  Fine-tune QoS, network filters, topic allow-listing/deny-listing and discovery via a human-readable YAML file.
- **Dynamic Types via XTypes**
Leverage [OMG DDS-XTypes 1.3](https://www.omg.org/spec/DDS-XTypes/1.3) and Fast DDS serialization utilities for runtime type registration and discovery.
- **Core Engine Powered by DDS-Pipe**
  Built on [eProsima DDS Pipe](https://github.com/eProsima/DDS-Pipe), ensuring low-latency, high-throughput payload forwarding and reliable discovery across distributed systems.
- **Serialization Utilities**
  Convert DDS data to JSON and vice versa for REST integration and to human-readable IDL.
- **Bidirectional FIWARE Integration**
  Routes DDS samples to NGSI-LD Context Brokers and injects context updates back into DDS topics.

![DDS Enabler Architecture](docs/rst/figures/ddsenabler_architecture.png)

## Commercial support

Looking for commercial support? Write us to info@eprosima.com

Find more about us at [eProsima’s webpage](https://eprosima.com/).

## Documentation

Explore the full user and developer guide hosted on ReadTheDocs:

- [Introduction](https://dds-enabler.readthedocs.io/en/latest/rst/02-formalia/titlepage.html)
- [Project Overview](https://dds-enabler.readthedocs.io/en/latest/rst/getting_started/project_overview.html)
- [User Manual](https://dds-enabler.readthedocs.io/en/latest/rst/user_manual/context_broker_interface.html)
- [API Reference](https://dds-enabler.readthedocs.io/en/latest/rst/ddsenabler/api_reference/api_reference.html)
