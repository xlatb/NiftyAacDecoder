#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "AacWindows.h"

namespace AacWindows
{
  // The windows are defined in § 15.3.2.

  // For the Kaiser-Bessel-derived (KBD) windows, the constants below contain
  //  the precalculated window function, called W'() by the standard. We do
  //  the final calculations for KBD_LEFT and KBD_RIGHT at runtime.

  // KBD constants for short windows. Length = 128, alpha = 6.
  static const double kbdShort[] =
  {
    0x1.2e5479bd8a574p-24,  0x1.df62c963d2ce9p-22,  0x1.81a64e78ce846p-20,  0x1.e4fac29e14082p-19,  0x1.09268bbaed42ap-17,  0x1.07ecceb65cf2fp-16,  0x1.ea499e3acd797p-16,  0x1.af798e66974fcp-15,
    0x1.6b71e7c364403p-14,  0x1.27235e10a58bep-13,  0x1.d0935c5492925p-13,  0x1.63d3c2f4b1732p-12,  0x1.0a114bb303c84p-11,  0x1.85750bc626982p-11,  0x1.17928c7cb83e6p-10,  0x1.8a60d6a85517fp-10,
    0x1.11b61417c060bp-9,   0x1.765040fb01ba5p-9,   0x1.f8de171a97008p-9,   0x1.501e5908d8594p-8,   0x1.ba2c9a1bde7bdp-8,   0x1.1f8fbbe14d05ep-7,   0x1.72081c1aa04ccp-7,   0x1.d7568b14e7a9dp-7,
    0x1.294db3230c8a7p-6,   0x1.739e7054cb4c7p-6,   0x1.cc70923b5f840p-6,   0x1.1ad97236f1dc0p-5,   0x1.58b51c817f554p-5,   0x1.a0d520f21eb60p-5,   0x1.f446791a33c98p-5,   0x1.2a0a1ca179d01p-4,
    0x1.60a0efea03061p-4,   0x1.9e628662bba26p-4,   0x1.e3bf5d6c07192p-4,   0x1.188d06ca513a6p-3,   0x1.4361ce8b35e1dp-3,   0x1.727be6b7a35c9p-3,   0x1.a5ecbe7563b88p-3,   0x1.ddb79b6137ea6p-3,
    0x1.0ce80364d61a8p-2,   0x1.2d0c370bfde06p-2,   0x1.4f30820e1c23bp-2,   0x1.73337815f4548p-2,   0x1.98e9eb6167812p-2,   0x1.c01ee2cfad804p-2,   0x1.e893c5977ada3p-2,   0x1.09005fa5798f4p-1,
    0x1.1e0aaeb0e0f74p-1,   0x1.333cb2fa8bbc6p-1,   0x1.4866f2dcb8b06p-1,   0x1.5d5738ce1d2a2p-1,   0x1.71d94b66af275p-1,   0x1.85b7b93aa325ep-1,   0x1.98bcb47a2ae5dp-1,   0x1.aab2f9aba4ae1p-1,
    0x1.bb66bc5bd6f87p-1,   0x1.caa694499b776p-1,   0x1.d8446563ed95dp-1,   0x1.e4163ced491c7p-1,   0x1.edf71e3ed15b0p-1,   0x1.f5c7b9f87dab0p-1,   0x1.fb6f0ae79f663p-1,   0x1.fedad48f1ee18p-1,
    0x1.0000000000000p+0,   0x1.fedad48f1ee18p-1,   0x1.fb6f0ae79f663p-1,   0x1.f5c7b9f87dab0p-1,   0x1.edf71e3ed15b0p-1,   0x1.e4163ced491c7p-1,   0x1.d8446563ed95dp-1,   0x1.caa694499b776p-1,
    0x1.bb66bc5bd6f87p-1,   0x1.aab2f9aba4ae1p-1,   0x1.98bcb47a2ae5dp-1,   0x1.85b7b93aa325ep-1,   0x1.71d94b66af275p-1,   0x1.5d5738ce1d2a2p-1,   0x1.4866f2dcb8b06p-1,   0x1.333cb2fa8bbc6p-1,
    0x1.1e0aaeb0e0f74p-1,   0x1.09005fa5798f4p-1,   0x1.e893c5977ada3p-2,   0x1.c01ee2cfad804p-2,   0x1.98e9eb6167812p-2,   0x1.73337815f4548p-2,   0x1.4f30820e1c23bp-2,   0x1.2d0c370bfde06p-2,
    0x1.0ce80364d61a8p-2,   0x1.ddb79b6137ea6p-3,   0x1.a5ecbe7563b88p-3,   0x1.727be6b7a35c9p-3,   0x1.4361ce8b35e1dp-3,   0x1.188d06ca513a6p-3,   0x1.e3bf5d6c07192p-4,   0x1.9e628662bba26p-4,
    0x1.60a0efea03061p-4,   0x1.2a0a1ca179d01p-4,   0x1.f446791a33c98p-5,   0x1.a0d520f21eb60p-5,   0x1.58b51c817f554p-5,   0x1.1ad97236f1dc0p-5,   0x1.cc70923b5f840p-6,   0x1.739e7054cb4c7p-6,
    0x1.294db3230c8a7p-6,   0x1.d7568b14e7a9dp-7,   0x1.72081c1aa04ccp-7,   0x1.1f8fbbe14d05ep-7,   0x1.ba2c9a1bde7bdp-8,   0x1.501e5908d8594p-8,   0x1.f8de171a97008p-9,   0x1.765040fb01ba5p-9,
    0x1.11b61417c060bp-9,   0x1.8a60d6a85517fp-10,  0x1.17928c7cb83e6p-10,  0x1.85750bc626982p-11,  0x1.0a114bb303c84p-11,  0x1.63d3c2f4b1732p-12,  0x1.d0935c5492925p-13,  0x1.27235e10a58bep-13,
    0x1.6b71e7c364403p-14,  0x1.af798e66974fcp-15,  0x1.ea499e3acd797p-16,  0x1.07ecceb65cf2fp-16,  0x1.09268bbaed42ap-17,  0x1.e4fac29e14082p-19,  0x1.81a64e78ce846p-20,  0x1.df62c963d2ce9p-22
  };

  static constexpr double kbdShortDenominator = 0x1.2598430995a72p+5;

  // KBD constants for long windows. Length = 1024, alpha = 4.
  static const double kbdLong[] =
  {
    0x1.014260ebdbbc3p-15,  0x1.2a7231730fb04p-15,  0x1.56c1958f46183p-15,  0x1.8655e078a9c11p-15,  0x1.b9558672c9563p-15,  0x1.efe821ef84d69p-15,  0x1.151b3c5b62806p-14,  0x1.343540894d84ap-14,
    0x1.5557b37fb2795p-14,  0x1.7898c8afc86fcp-14,  0x1.9e0f53994bd9cp-14,  0x1.c5d2ca6762b75p-14,  0x1.effb488eb1d5ap-14,  0x1.0e50c8b636307p-13,  0x1.25ef897313058p-13,  0x1.3ee6f4051793cp-13,
    0x1.59446dd82087bp-13,  0x1.7515b58d2024bp-13,  0x1.9268e44a6c7ddp-13,  0x1.b14c6f0bdc9cfp-13,  0x1.d1cf27f295f64p-13,  0x1.f4003f946b0abp-13,  0x1.0bf7a32555d92p-12,  0x1.1ed616c0238a7p-12,
    0x1.32a3a47f8f736p-12,  0x1.4768a81e358dcp-12,  0x1.5d2dafd206664p-12,  0x1.73fb7cf157564p-12,  0x1.8bdb049748823p-12,  0x1.a4d570476f5cap-12,  0x1.bef41e90b449dp-12,  0x1.da40a3af51c84p-12,
    0x1.f6c4ca2de3504p-12,  0x1.0a4549c2b8fd2p-11,  0x1.19ce1c5e365e6p-11,  0x1.2a0215823cffcp-11,  0x1.3ae68a2b87fafp-11,  0x1.4c80ec0860958p-11,  0x1.5ed6c9c595384p-11,  0x1.71edcf5ab7954p-11,
    0x1.85cbc655987ddp-11,  0x1.9a769624f7ba5p-11,  0x1.aff444625e3fcp-11,  0x1.c64af51b18f8bp-11,  0x1.dd80eb184a53ep-11,  0x1.f59c882608b67p-11,  0x1.075226acbff2dp-10,  0x1.144f6dab05af9p-10,
    0x1.21c979489fd3fp-10,  0x1.2fc3b9caec33cp-10,  0x1.3e41afa2c09bdp-10,  0x1.4d46eb8cb77dfp-10,  0x1.5cd70eb0dfc14p-10,  0x1.6cf5cac1ca995p-10,  0x1.7da6e21af23f3p-10,  0x1.8eee27de7465ep-10,
    0x1.a0cf80121b3a8p-10,  0x1.b34edfbbafc69p-10,  0x1.c6704cfc90804p-10,  0x1.da37df2c86d79p-10,  0x1.eea9bef3d68ddp-10,  0x1.01e5133240ceep-9,   0x1.0cceb0895dc1cp-9,   0x1.1813e6164354ep-9,
    0x1.23b6eb483b663p-9,   0x1.2fba0071c98a6p-9,   0x1.3c1f6ed329625p-9,   0x1.48e988a4579aap-9,   0x1.561aa91ea300bp-9,   0x1.63b53485c3178p-9,   0x1.71bb98307193cp-9,   0x1.80304a9084333p-9,
    0x1.8f15cb3a8454bp-9,   0x1.9e6ea2ecc1d1fp-9,   0x1.ae3d6395de7ccp-9,   0x1.be84a85acfca2p-9,   0x1.cf47159c5416dp-9,   0x1.e08758fbd90d0p-9,   0x1.f248295fd0aabp-9,   0x1.0246237bb9356p-8,
    0x1.0bab3d9ef3122p-8,   0x1.1554cc7e6b0fbp-8,   0x1.1f443e273304bp-8,   0x1.297b054f4622bp-8,   0x1.33fa9955e5ab8p-8,   0x1.3ec47643a473fp-8,   0x1.49da1cca20199p-8,   0x1.553d124366b87p-8,
    0x1.60eee0b107fa1p-8,   0x1.6cf116bad06bdp-8,   0x1.794547ad2de7bp-8,   0x1.85ed0b773c08dp-8,   0x1.92e9fea8778d0p-8,   0x1.a03dc26e1787cp-8,   0x1.ade9fc900b6cap-8,   0x1.bbf0576d9cd3fp-8,
    0x1.ca5281f9b3f83p-8,   0x1.d9122fb6bdf1fp-8,   0x1.e83118b233ac1p-8,   0x1.f7b0f97fc0984p-8,   0x1.03c9c99a041cbp-7,   0x1.0bed55af84c28p-7,   0x1.144406030fa2bp-7,   0x1.1ccec1cece44cp-7,
    0x1.258e728102215p-7,   0x1.2e8403b86da57p-7,   0x1.37b0634089f1fp-7,   0x1.4114810d88f8dp-7,   0x1.4ab14f3823966p-7,   0x1.5487c1f9333c1p-7,   0x1.5e98cfa516da1p-7,   0x1.68e570a6e29fep-7,
    0x1.736e9f7b5a4d0p-7,   0x1.7e3558abb5ad1p-7,   0x1.893a9ac82ef77p-7,   0x1.947f66625ac3ap-7,   0x1.a004be074949ep-7,   0x1.abcba639709f8p-7,   0x1.b7d5256a5fbcfp-7,   0x1.c42243f439ff3p-7,
    0x1.d0b40c12faecap-7,   0x1.dd8b89dd8210dp-7,   0x1.eaa9cb3e66a3ap-7,   0x1.f80fdfec92e59p-7,   0x1.02df6cb1d378ap-6,   0x1.09dbe56e1172cp-6,   0x1.10fde4a1ac1e4p-6,   0x1.1845f5999118dp-6,
    0x1.1fb4a474b28f9p-6,   0x1.274a7e1faa59fp-6,   0x1.2f08105040790p-6,   0x1.36ede980d4f0ap-6,   0x1.3efc98ebacfd7p-6,   0x1.4734ae862393cp-6,   0x1.4f96bafbbd3b7p-6,   0x1.58234fa91f35cp-6,
    0x1.60dafe96e9fe8p-6,   0x1.69be5a7477289p-6,   0x1.72cdf6927a98dp-6,   0x1.7c0a66dd8735fp-6,   0x1.85743fd8770eep-6,   0x1.8f0c1696b706ep-6,   0x1.98d280b676227p-6,   0x1.a2c8145ab8798p-6,
    0x1.aced68254df71p-6,   0x1.b7431330ace98p-6,   0x1.c1c9ad09b0922p-6,   0x1.cc81cda93bc38p-6,   0x1.d76c0d6dbfb10p-6,   0x1.e2890514a71e6p-6,   0x1.edd94db3a5f99p-6,   0x1.f95d80b1eda09p-6,
    0x1.028b1be0a2f74p-5,   0x1.0882066b859e4p-5,   0x1.0e93cd1288c50p-5,   0x1.14c0bd09367d2p-5,   0x1.1b09239a0feecp-5,   0x1.216d4e22d737ep-5,   0x1.27ed8a10cc4f8p-5,   0x1.2e8a24dcdcfa0p-5,
    0x1.35436c07c7ff4p-5,   0x1.3c19ad1633ae5p-5,   0x1.430d358cb7e45p-5,   0x1.4a1e52ebdbabdp-5,   0x1.514d52ac069c2p-5,   0x1.589a8239661e1p-5,   0x1.60062eefc6c30p-5,   0x1.6790a61661c51p-5,
    0x1.6f3a34db9eedbp-5,   0x1.77032850cafe3p-5,   0x1.7eebcd65c2cc5p-5,   0x1.86f470e4933ffp-5,   0x1.8f1d5f6d0e5c8p-5,   0x1.9766e57055880p-5,   0x1.9fd14f2c593eap-5,   0x1.a85ce8a74e65bp-5,
    0x1.b109fdab19745p-5,   0x1.b9d8d9c0af9dbp-5,   0x1.c2c9c82b6e3cdp-5,   0x1.cbdd13e468aa5p-5,   0x1.d5130795acc96p-5,   0x1.de6bed957e6efp-5,   0x1.e7e80fe189e88p-5,   0x1.f187b81a0de58p-5,
    0x1.fb4b2f7cfce6ap-5,   0x1.02995f708b4b1p-4,   0x1.079f57587c8f5p-4,   0x1.0cb7a37315739p-4,   0x1.11e267820e6c5p-4,   0x1.171fc70992c27p-4,   0x1.1c6fe54dbaad2p-4,   0x1.21d2e55002241p-4,
    0x1.2748e9ccbc7dap-4,   0x1.2cd215388507bp-4,   0x1.326e89bdacb5fp-4,   0x1.381e6939a516dp-4,   0x1.3de1d53a68a25p-4,   0x1.43b8eefbe0976p-4,   0x1.49a3d765488a4p-4,   0x1.4fa2af068fc4dp-4,
    0x1.55b59615b8a3dp-4,   0x1.5bdcac6c36196p-4,   0x1.62181184477dap-4,   0x1.6867e47652d12p-4,   0x1.6ecc43f63d9b6p-4,   0x1.75454e50c4948p-4,   0x1.7bd32168d23c7p-4,   0x1.8275dab4d48c2p-4,
    0x1.892d973c11eecp-4,   0x1.8ffa7393fda8ap-4,   0x1.96dc8bdd8bdb3p-4,   0x1.9dd3fbc2854e9p-4,   0x1.a4e0de72db31ap-4,   0x1.ac034ea1faf60p-4,   0x1.b33b6684227bap-4,   0x1.ba893fcbb4b86p-4,
    0x1.c1ecf3a68eff3p-4,   0x1.c9669abb5f287p-4,   0x1.d0f64d26fab1ap-4,   0x1.d89c2279b71a4p-4,   0x1.e05831b4c3945p-4,   0x1.e82a914784550p-4,   0x1.f013570cef955p-4,   0x1.f8129848ec87ap-4,
    0x1.001434d2da36fp-3,   0x1.042a6f989afcep-3,   0x1.084c062d3d93ap-3,   0x1.0c7901f788e83p-3,   0x1.10b16c0c3b8e2p-3,   0x1.14f54d2cc2fa5p-3,   0x1.1944adc5f4624p-3,   0x1.1d9f95eec7577p-3,
    0x1.22060d67122c2p-3,   0x1.26781b9648471p-3,   0x1.2af5c78a3a7a0p-3,   0x1.2f7f17f5d970bp-3,   0x1.3414132ffa579p-3,   0x1.38b4bf321dcb6p-3,   0x1.3d61219739305p-3,   0x1.42193f9a82892p-3,
    0x1.46dd1e163ee08p-3,   0x1.4bacc182936a5p-3,   0x1.50882df459620p-3,   0x1.556f671bf4d68p-3,   0x1.5a6270442e69fp-3,   0x1.5f614c511022fp-3,   0x1.646bfdbec56e8p-3,   0x1.698286a07e62fp-3,
    0x1.6ea4e89f565cdp-3,   0x1.73d324f93e196p-3,   0x1.790d3c7fe955cp-3,   0x1.7e532f97c0197p-3,   0x1.83a4fe36d3be0p-3,   0x1.8902a7e3d7cb8p-3,   0x1.8e6c2bb51ec52p-3,   0x1.93e1884f9afc5p-3,
    0x1.9962bbe5e3815p-3,   0x1.9eefc4373d4adp-3,   0x1.a4889e8ea8a84p-3,   0x1.aa2d47c1f31f1p-3,   0x1.afddbc30cdbbcp-3,   0x1.b599f7c3e7fe2p-3,   0x1.bb61f5ec0f6d5p-3,   0x1.c135b1a153e8cp-3,
    0x1.c715256230da2p-3,   0x1.cd004b32bb48cp-3,   0x1.d2f71c9bd5016p-3,   0x1.d8f992aa64c70p-3,   0x1.df07a5ee93c20p-3,   0x1.e5214e7b102f6p-3,   0x1.eb4683e455670p-3,   0x1.f1773d3ff95d2p-3,
    0x1.f7b37123ff9c4p-3,   0x1.fdfb15a631e41p-3,   0x1.0227102dbf378p-2,   0x1.0556432badff2p-2,   0x1.088b1e1599dc0p-2,   0x1.0bc59af2e87bap-2,   0x1.0f05b3896b859p-2,   0x1.124b615d1e8b9p-2,
    0x1.15969dafe8041p-2,   0x1.18e761815d612p-2,   0x1.1c3da58e8a4f4p-2,   0x1.1f996251bb17cp-2,   0x1.22fa90024a41bp-2,   0x1.26612694716b8p-2,   0x1.29cd1db91d694p-2,   0x1.2d3e6cddc5b44p-2,
    0x1.30b50b2c47315p-2,   0x1.3430ef8ac251dp-2,   0x1.37b2109b7c9d1p-2,   0x1.3b3864bcc5a44p-2,   0x1.3ec3e208df6f4p-2,   0x1.42547e55ea5f8p-2,   0x1.45ea2f35d4989p-2,   0x1.4984e9f64cf2bp-2,
    0x1.4d24a3a0b977bp-2,   0x1.50c950fa317b8p-2,   0x1.5472e6837b4f1p-2,   0x1.582158790d8dfp-2,   0x1.5bd49ad3141f1p-2,   0x1.5f8ca14578d93p-2,   0x1.63495f3fefdd2p-2,   0x1.670ac7ee07a38p-2,
    0x1.6ad0ce373cc98p-2,   0x1.6e9b64bf119d1p-2,   0x1.726a7de5296b9p-2,   0x1.763e0bc567977p-2,   0x1.7a16003812824p-2,   0x1.7df24cd1fa3afp-2,   0x1.81d2e2e4a3076p-2,   0x1.85b7b37e73c07p-2,
    0x1.89a0af6ae80adp-2,   0x1.8d8dc732c6642p-2,   0x1.917eeb1c5a15dp-2,   0x1.95740b2bb1054p-2,   0x1.996d1722dd619p-2,   0x1.9d69fe823b3e1p-2,   0x1.a16ab088ba088p-2,   0x1.a56f1c3429e91p-2,
    0x1.a97730418d0a5p-2,   0x1.ad82db2d6cc70p-2,   0x1.b1920b3432be9p-2,   0x1.b5a4ae5285d62p-2,   0x1.b9bab245ab167p-2,   0x1.bdd4048bea7ecp-2,   0x1.c1f09264f7b34p-2,   0x1.c61048d25e953p-2,
    0x1.ca331497f3bcep-2,   0x1.ce58e23c48d97p-2,   0x1.d2819e0924efbp-2,   0x1.d6ad340c00750p-2,   0x1.dadb90168548ap-2,   0x1.df0c9dbf12860p-2,   0x1.e340486144332p-2,   0x1.e7767b1e7eb4ap-2,
    0x1.ebaf20de7e218p-2,   0x1.efea244fe95acp-2,   0x1.f4276fe8e8ec2p-2,   0x1.f866ede7c1b51p-2,   0x1.fca8885373472p-2,   0x1.0076147e2d088p-1,   0x1.0298dcbe6a93fp-1,   0x1.04bc919cf7ee0p-1,
    0x1.06e127b2076f7p-1,   0x1.0906937bd08fbp-1,   0x1.0b2cc95eea1f1p-1,   0x1.0d53bda6a64d4p-1,   0x1.0f7b648570787p-1,   0x1.11a3b2152cc62p-1,   0x1.13cc9a579974ep-1,   0x1.15f61136b1f52p-1,
    0x1.18200a8513b64p-1,   0x1.1a4a79fe64affp-1,   0x1.1c755347bb93dp-1,   0x1.1ea089f009b88p-1,   0x1.20cc117086a5fp-1,   0x1.22f7dd2d1d493p-1,   0x1.2523e074dac8dp-1,   0x1.27500e825ef57p-1,
    0x1.297c5a7c4e4eap-1,   0x1.2ba8b775c59bap-1,   0x1.2dd5186ecf109p-1,   0x1.30017054d8f7dp-1,   0x1.322db2032de07p-1,   0x1.3459d0436e4a8p-1,   0x1.3685bdce0bc25p-1,   0x1.38b16d4ac576ap-1,
    0x1.3adcd15126314p-1,   0x1.3d07dc6903bb9p-1,   0x1.3f32810aff9e4p-1,   0x1.415cb1a10937ep-1,   0x1.43866086e12dap-1,   0x1.45af800a9e1f5p-1,   0x1.47d8026d32a5fp-1,   0x1.49ffd9e2f4908p-1,
    0x1.4c26f89425576p-1,   0x1.4e4d509d7bc4ap-1,   0x1.5072d410aebe2p-1,   0x1.529774f501411p-1,   0x1.54bb2547cf6adp-1,   0x1.56ddd6fd1c9fdp-1,   0x1.58ff7c0022bd7p-1,   0x1.5b200633e250dp-1,
    0x1.5d3f6773b3ce3p-1,   0x1.5f5d9193d9bdfp-1,   0x1.617a766213d9ap-1,   0x1.639607a6330ebp-1,   0x1.65b03722ae618p-1,   0x1.67c8f69538ab9p-1,   0x1.69e037b75721bp-1,   0x1.6bf5ec3ef8a8ap-1,
    0x1.6e0a05df0de50p-1,   0x1.701c764822023p-1,   0x1.722d2f28f42bep-1,   0x1.743c222f11a7dp-1,   0x1.76494107708e7p-1,   0x1.78547d5f0b1ffp-1,   0x1.7a5dc8e37b973p-1,   0x1.7c6515439892dp-1,
    0x1.7e6a543011e72p-1,   0x1.806d775c0df26p-1,   0x1.826e707dc74a0p-1,   0x1.846d314f2ad8cp-1,   0x1.8669ab8e764a4p-1,   0x1.8863d0fed6c62p-1,   0x1.8a5b936907f33p-1,   0x1.8c50e49bf336ep-1,
    0x1.8e43b66d4f23ep-1,   0x1.9033faba3f19cp-1,   0x1.9221a367f2feep-1,   0x1.940ca26447179p-1,   0x1.95f4e9a663e3dp-1,   0x1.97da6b2f5e0d7p-1,   0x1.99bd190ad6478p-1,   0x1.9b9ce54f9926cp-1,
    0x1.9d79c2203edffp-1,   0x1.9f53a1abcae55p-1,   0x1.a12a762e4b607p-1,   0x1.a2fe31f1786e4p-1,   0x1.a4cec74d532c4p-1,   0x1.a69c28a8c4720p-1,   0x1.a866487a3b457p-1,   0x1.aa2d19484aefdp-1,
    0x1.abf08daa48b86p-1,   0x1.adb09848e928bp-1,   0x1.af6d2bdedce64p-1,   0x1.b1263b396d06fp-1,   0x1.b2dbb93916e15p-1,   0x1.b48d98d227483p-1,   0x1.b63bcd0d552f4p-1,   0x1.b7e649085bacdp-1,
    0x1.b98cfff693466p-1,   0x1.bb2fe5218a91ep-1,   0x1.bcceebe99e0fep-1,   0x1.be6a07c68f393p-1,   0x1.c0012c481ac92p-1,   0x1.c1944d168e197p-1,   0x1.c3235df35bab2p-1,   0x1.c4ae52b9aeb12p-1,
    0x1.c6351f5efdb32p-1,   0x1.c7b7b7f39c290p-1,   0x1.c93610a34b0b3p-1,   0x1.cab01db5c85b8p-1,   0x1.cc25d38f5d85bp-1,   0x1.cd9726b16cab1p-1,   0x1.cf040bbafcb83p-1,   0x1.d06c77694446ep-1,
    0x1.d1d05e98334e4p-1,   0x1.d32fb642fb765p-1,   0x1.d48a73849736dp-1,   0x1.d5e08b984f90ep-1,   0x1.d731f3da4067ap-1,   0x1.d87ea1c7db7b2p-1,   0x1.d9c68b0069ebep-1,   0x1.db09a5458c4eap-1,
    0x1.dc47e67bb9366p-1,   0x1.dd8144aaba405p-1,   0x1.deb5b5fe278b5p-1,   0x1.dfe530c5e1977p-1,   0x1.e10fab76898dep-1,   0x1.e2351ca9f7d7fp-1,   0x1.e3557b1fb1119p-1,   0x1.e470bdbd59426p-1,
    0x1.e586db8f25579p-1,   0x1.e697cbc84ae31p-1,   0x1.e7a385c36e036p-1,   0x1.e8aa01030d8a3p-1,   0x1.e9ab3531ed35bp-1,   0x1.eaa71a237e1e5p-1,   0x1.eb9da7d445361p-1,   0x1.ec8ed66a3fd56p-1,
    0x1.ed7a9e3546628p-1,   0x1.ee60f7af6cf88p-1,   0x1.ef41db7d62095p-1,   0x1.f01d426ecb08fp-1,   0x1.f0f3257e9effep-1,   0x1.f1c37dd37f172p-1,   0x1.f28e44c00d030p-1,   0x1.f35373c33f5dep-1,
    0x1.f4130488b3d74p-1,   0x1.f4ccf0e8ff455p-1,   0x1.f58132e9fb7f5p-1,   0x1.f62fc4bf13115p-1,   0x1.f6d8a0c98ab7dp-1,   0x1.f77bc198c89f8p-1,   0x1.f81921ea99629p-1,   0x1.f8b0bcab72cc4p-1,
    0x1.f9428cf6b44bbp-1,   0x1.f9ce8e16e521ap-1,   0x1.fa54bb85f0382p-1,   0x1.fad510ed5daacp-1,   0x1.fb4f8a2689facp-1,   0x1.fbc4233adade0p-1,   0x1.fc32d863f1ba3p-1,   0x1.fc9ba60bdbbcep-1,
    0x1.fcfe88cd3f875p-1,   0x1.fd5b7d7388793p-1,   0x1.fdb280fb0f95ep-1,   0x1.fe03909141ee9p-1,   0x1.fe4ea994c4aebp-1,   0x1.fe93c99596a54p-1,   0x1.fed2ee552f670p-1,   0x1.ff0c15c69bf36p-1,
    0x1.ff3f3e0e98eb5p-1,   0x1.ff6c6583aa3fcp-1,   0x1.ff938aae30723p-1,   0x1.ffb4ac487b537p-1,   0x1.ffcfc93eda44bp-1,   0x1.ffe4e0afa9fd9p-1,   0x1.fff3f1eb5fccfp-1,   0x1.fffcfc74925f6p-1,
    0x1.0000000000000p+0,   0x1.fffcfc74925f6p-1,   0x1.fff3f1eb5fccfp-1,   0x1.ffe4e0afa9fd9p-1,   0x1.ffcfc93eda44bp-1,   0x1.ffb4ac487b537p-1,   0x1.ff938aae30723p-1,   0x1.ff6c6583aa3fcp-1,
    0x1.ff3f3e0e98eb5p-1,   0x1.ff0c15c69bf36p-1,   0x1.fed2ee552f670p-1,   0x1.fe93c99596a54p-1,   0x1.fe4ea994c4aebp-1,   0x1.fe03909141ee9p-1,   0x1.fdb280fb0f95ep-1,   0x1.fd5b7d7388793p-1,
    0x1.fcfe88cd3f875p-1,   0x1.fc9ba60bdbbcep-1,   0x1.fc32d863f1ba3p-1,   0x1.fbc4233adade0p-1,   0x1.fb4f8a2689facp-1,   0x1.fad510ed5daacp-1,   0x1.fa54bb85f0382p-1,   0x1.f9ce8e16e521ap-1,
    0x1.f9428cf6b44bbp-1,   0x1.f8b0bcab72cc4p-1,   0x1.f81921ea99629p-1,   0x1.f77bc198c89f8p-1,   0x1.f6d8a0c98ab7dp-1,   0x1.f62fc4bf13115p-1,   0x1.f58132e9fb7f5p-1,   0x1.f4ccf0e8ff455p-1,
    0x1.f4130488b3d74p-1,   0x1.f35373c33f5dep-1,   0x1.f28e44c00d030p-1,   0x1.f1c37dd37f172p-1,   0x1.f0f3257e9effep-1,   0x1.f01d426ecb08fp-1,   0x1.ef41db7d62095p-1,   0x1.ee60f7af6cf88p-1,
    0x1.ed7a9e3546628p-1,   0x1.ec8ed66a3fd56p-1,   0x1.eb9da7d445361p-1,   0x1.eaa71a237e1e5p-1,   0x1.e9ab3531ed35bp-1,   0x1.e8aa01030d8a3p-1,   0x1.e7a385c36e036p-1,   0x1.e697cbc84ae31p-1,
    0x1.e586db8f25579p-1,   0x1.e470bdbd59426p-1,   0x1.e3557b1fb1119p-1,   0x1.e2351ca9f7d7fp-1,   0x1.e10fab76898dep-1,   0x1.dfe530c5e1977p-1,   0x1.deb5b5fe278b5p-1,   0x1.dd8144aaba405p-1,
    0x1.dc47e67bb9366p-1,   0x1.db09a5458c4eap-1,   0x1.d9c68b0069ebep-1,   0x1.d87ea1c7db7b2p-1,   0x1.d731f3da4067ap-1,   0x1.d5e08b984f90ep-1,   0x1.d48a73849736dp-1,   0x1.d32fb642fb765p-1,
    0x1.d1d05e98334e4p-1,   0x1.d06c77694446ep-1,   0x1.cf040bbafcb83p-1,   0x1.cd9726b16cab1p-1,   0x1.cc25d38f5d85bp-1,   0x1.cab01db5c85b8p-1,   0x1.c93610a34b0b3p-1,   0x1.c7b7b7f39c290p-1,
    0x1.c6351f5efdb32p-1,   0x1.c4ae52b9aeb12p-1,   0x1.c3235df35bab2p-1,   0x1.c1944d168e197p-1,   0x1.c0012c481ac92p-1,   0x1.be6a07c68f393p-1,   0x1.bcceebe99e0fep-1,   0x1.bb2fe5218a91ep-1,
    0x1.b98cfff693466p-1,   0x1.b7e649085bacdp-1,   0x1.b63bcd0d552f4p-1,   0x1.b48d98d227483p-1,   0x1.b2dbb93916e15p-1,   0x1.b1263b396d06fp-1,   0x1.af6d2bdedce64p-1,   0x1.adb09848e928bp-1,
    0x1.abf08daa48b86p-1,   0x1.aa2d19484aefdp-1,   0x1.a866487a3b457p-1,   0x1.a69c28a8c4720p-1,   0x1.a4cec74d532c4p-1,   0x1.a2fe31f1786e4p-1,   0x1.a12a762e4b607p-1,   0x1.9f53a1abcae55p-1,
    0x1.9d79c2203edffp-1,   0x1.9b9ce54f9926cp-1,   0x1.99bd190ad6478p-1,   0x1.97da6b2f5e0d7p-1,   0x1.95f4e9a663e3dp-1,   0x1.940ca26447179p-1,   0x1.9221a367f2feep-1,   0x1.9033faba3f19cp-1,
    0x1.8e43b66d4f23ep-1,   0x1.8c50e49bf336ep-1,   0x1.8a5b936907f33p-1,   0x1.8863d0fed6c62p-1,   0x1.8669ab8e764a4p-1,   0x1.846d314f2ad8cp-1,   0x1.826e707dc74a0p-1,   0x1.806d775c0df26p-1,
    0x1.7e6a543011e72p-1,   0x1.7c6515439892dp-1,   0x1.7a5dc8e37b973p-1,   0x1.78547d5f0b1ffp-1,   0x1.76494107708e7p-1,   0x1.743c222f11a7dp-1,   0x1.722d2f28f42bep-1,   0x1.701c764822023p-1,
    0x1.6e0a05df0de50p-1,   0x1.6bf5ec3ef8a8ap-1,   0x1.69e037b75721bp-1,   0x1.67c8f69538ab9p-1,   0x1.65b03722ae618p-1,   0x1.639607a6330ebp-1,   0x1.617a766213d9ap-1,   0x1.5f5d9193d9bdfp-1,
    0x1.5d3f6773b3ce3p-1,   0x1.5b200633e250dp-1,   0x1.58ff7c0022bd7p-1,   0x1.56ddd6fd1c9fdp-1,   0x1.54bb2547cf6adp-1,   0x1.529774f501411p-1,   0x1.5072d410aebe2p-1,   0x1.4e4d509d7bc4ap-1,
    0x1.4c26f89425576p-1,   0x1.49ffd9e2f4908p-1,   0x1.47d8026d32a5fp-1,   0x1.45af800a9e1f5p-1,   0x1.43866086e12dap-1,   0x1.415cb1a10937ep-1,   0x1.3f32810aff9e4p-1,   0x1.3d07dc6903bb9p-1,
    0x1.3adcd15126314p-1,   0x1.38b16d4ac576ap-1,   0x1.3685bdce0bc25p-1,   0x1.3459d0436e4a8p-1,   0x1.322db2032de07p-1,   0x1.30017054d8f7dp-1,   0x1.2dd5186ecf109p-1,   0x1.2ba8b775c59bap-1,
    0x1.297c5a7c4e4eap-1,   0x1.27500e825ef57p-1,   0x1.2523e074dac8dp-1,   0x1.22f7dd2d1d493p-1,   0x1.20cc117086a5fp-1,   0x1.1ea089f009b88p-1,   0x1.1c755347bb93dp-1,   0x1.1a4a79fe64affp-1,
    0x1.18200a8513b64p-1,   0x1.15f61136b1f52p-1,   0x1.13cc9a579974ep-1,   0x1.11a3b2152cc62p-1,   0x1.0f7b648570787p-1,   0x1.0d53bda6a64d4p-1,   0x1.0b2cc95eea1f1p-1,   0x1.0906937bd08fbp-1,
    0x1.06e127b2076f7p-1,   0x1.04bc919cf7ee0p-1,   0x1.0298dcbe6a93fp-1,   0x1.0076147e2d088p-1,   0x1.fca8885373472p-2,   0x1.f866ede7c1b51p-2,   0x1.f4276fe8e8ec2p-2,   0x1.efea244fe95acp-2,
    0x1.ebaf20de7e218p-2,   0x1.e7767b1e7eb4ap-2,   0x1.e340486144332p-2,   0x1.df0c9dbf12860p-2,   0x1.dadb90168548ap-2,   0x1.d6ad340c00750p-2,   0x1.d2819e0924efbp-2,   0x1.ce58e23c48d97p-2,
    0x1.ca331497f3bcep-2,   0x1.c61048d25e953p-2,   0x1.c1f09264f7b34p-2,   0x1.bdd4048bea7ecp-2,   0x1.b9bab245ab167p-2,   0x1.b5a4ae5285d62p-2,   0x1.b1920b3432be9p-2,   0x1.ad82db2d6cc70p-2,
    0x1.a97730418d0a5p-2,   0x1.a56f1c3429e91p-2,   0x1.a16ab088ba088p-2,   0x1.9d69fe823b3e1p-2,   0x1.996d1722dd619p-2,   0x1.95740b2bb1054p-2,   0x1.917eeb1c5a15dp-2,   0x1.8d8dc732c6642p-2,
    0x1.89a0af6ae80adp-2,   0x1.85b7b37e73c07p-2,   0x1.81d2e2e4a3076p-2,   0x1.7df24cd1fa3afp-2,   0x1.7a16003812824p-2,   0x1.763e0bc567977p-2,   0x1.726a7de5296b9p-2,   0x1.6e9b64bf119d1p-2,
    0x1.6ad0ce373cc98p-2,   0x1.670ac7ee07a38p-2,   0x1.63495f3fefdd2p-2,   0x1.5f8ca14578d93p-2,   0x1.5bd49ad3141f1p-2,   0x1.582158790d8dfp-2,   0x1.5472e6837b4f1p-2,   0x1.50c950fa317b8p-2,
    0x1.4d24a3a0b977bp-2,   0x1.4984e9f64cf2bp-2,   0x1.45ea2f35d4989p-2,   0x1.42547e55ea5f8p-2,   0x1.3ec3e208df6f4p-2,   0x1.3b3864bcc5a44p-2,   0x1.37b2109b7c9d1p-2,   0x1.3430ef8ac251dp-2,
    0x1.30b50b2c47315p-2,   0x1.2d3e6cddc5b44p-2,   0x1.29cd1db91d694p-2,   0x1.26612694716b8p-2,   0x1.22fa90024a41bp-2,   0x1.1f996251bb17cp-2,   0x1.1c3da58e8a4f4p-2,   0x1.18e761815d612p-2,
    0x1.15969dafe8041p-2,   0x1.124b615d1e8b9p-2,   0x1.0f05b3896b859p-2,   0x1.0bc59af2e87bap-2,   0x1.088b1e1599dc0p-2,   0x1.0556432badff2p-2,   0x1.0227102dbf378p-2,   0x1.fdfb15a631e41p-3,
    0x1.f7b37123ff9c4p-3,   0x1.f1773d3ff95d2p-3,   0x1.eb4683e455670p-3,   0x1.e5214e7b102f6p-3,   0x1.df07a5ee93c20p-3,   0x1.d8f992aa64c70p-3,   0x1.d2f71c9bd5016p-3,   0x1.cd004b32bb48cp-3,
    0x1.c715256230da2p-3,   0x1.c135b1a153e8cp-3,   0x1.bb61f5ec0f6d5p-3,   0x1.b599f7c3e7fe2p-3,   0x1.afddbc30cdbbcp-3,   0x1.aa2d47c1f31f1p-3,   0x1.a4889e8ea8a84p-3,   0x1.9eefc4373d4adp-3,
    0x1.9962bbe5e3815p-3,   0x1.93e1884f9afc5p-3,   0x1.8e6c2bb51ec52p-3,   0x1.8902a7e3d7cb8p-3,   0x1.83a4fe36d3be0p-3,   0x1.7e532f97c0197p-3,   0x1.790d3c7fe955cp-3,   0x1.73d324f93e196p-3,
    0x1.6ea4e89f565cdp-3,   0x1.698286a07e62fp-3,   0x1.646bfdbec56e8p-3,   0x1.5f614c511022fp-3,   0x1.5a6270442e69fp-3,   0x1.556f671bf4d68p-3,   0x1.50882df459620p-3,   0x1.4bacc182936a5p-3,
    0x1.46dd1e163ee08p-3,   0x1.42193f9a82892p-3,   0x1.3d61219739305p-3,   0x1.38b4bf321dcb6p-3,   0x1.3414132ffa579p-3,   0x1.2f7f17f5d970bp-3,   0x1.2af5c78a3a7a0p-3,   0x1.26781b9648471p-3,
    0x1.22060d67122c2p-3,   0x1.1d9f95eec7577p-3,   0x1.1944adc5f4624p-3,   0x1.14f54d2cc2fa5p-3,   0x1.10b16c0c3b8e2p-3,   0x1.0c7901f788e83p-3,   0x1.084c062d3d93ap-3,   0x1.042a6f989afcep-3,
    0x1.001434d2da36fp-3,   0x1.f8129848ec87ap-4,   0x1.f013570cef955p-4,   0x1.e82a914784550p-4,   0x1.e05831b4c3945p-4,   0x1.d89c2279b71a4p-4,   0x1.d0f64d26fab1ap-4,   0x1.c9669abb5f287p-4,
    0x1.c1ecf3a68eff3p-4,   0x1.ba893fcbb4b86p-4,   0x1.b33b6684227bap-4,   0x1.ac034ea1faf60p-4,   0x1.a4e0de72db31ap-4,   0x1.9dd3fbc2854e9p-4,   0x1.96dc8bdd8bdb3p-4,   0x1.8ffa7393fda8ap-4,
    0x1.892d973c11eecp-4,   0x1.8275dab4d48c2p-4,   0x1.7bd32168d23c7p-4,   0x1.75454e50c4948p-4,   0x1.6ecc43f63d9b6p-4,   0x1.6867e47652d12p-4,   0x1.62181184477dap-4,   0x1.5bdcac6c36196p-4,
    0x1.55b59615b8a3dp-4,   0x1.4fa2af068fc4dp-4,   0x1.49a3d765488a4p-4,   0x1.43b8eefbe0976p-4,   0x1.3de1d53a68a25p-4,   0x1.381e6939a516dp-4,   0x1.326e89bdacb5fp-4,   0x1.2cd215388507bp-4,
    0x1.2748e9ccbc7dap-4,   0x1.21d2e55002241p-4,   0x1.1c6fe54dbaad2p-4,   0x1.171fc70992c27p-4,   0x1.11e267820e6c5p-4,   0x1.0cb7a37315739p-4,   0x1.079f57587c8f5p-4,   0x1.02995f708b4b1p-4,
    0x1.fb4b2f7cfce6ap-5,   0x1.f187b81a0de58p-5,   0x1.e7e80fe189e88p-5,   0x1.de6bed957e6efp-5,   0x1.d5130795acc96p-5,   0x1.cbdd13e468aa5p-5,   0x1.c2c9c82b6e3cdp-5,   0x1.b9d8d9c0af9dbp-5,
    0x1.b109fdab19745p-5,   0x1.a85ce8a74e65bp-5,   0x1.9fd14f2c593eap-5,   0x1.9766e57055880p-5,   0x1.8f1d5f6d0e5c8p-5,   0x1.86f470e4933ffp-5,   0x1.7eebcd65c2cc5p-5,   0x1.77032850cafe3p-5,
    0x1.6f3a34db9eedbp-5,   0x1.6790a61661c51p-5,   0x1.60062eefc6c30p-5,   0x1.589a8239661e1p-5,   0x1.514d52ac069c2p-5,   0x1.4a1e52ebdbabdp-5,   0x1.430d358cb7e45p-5,   0x1.3c19ad1633ae5p-5,
    0x1.35436c07c7ff4p-5,   0x1.2e8a24dcdcfa0p-5,   0x1.27ed8a10cc4f8p-5,   0x1.216d4e22d737ep-5,   0x1.1b09239a0feecp-5,   0x1.14c0bd09367d2p-5,   0x1.0e93cd1288c50p-5,   0x1.0882066b859e4p-5,
    0x1.028b1be0a2f74p-5,   0x1.f95d80b1eda09p-6,   0x1.edd94db3a5f99p-6,   0x1.e2890514a71e6p-6,   0x1.d76c0d6dbfb10p-6,   0x1.cc81cda93bc38p-6,   0x1.c1c9ad09b0922p-6,   0x1.b7431330ace98p-6,
    0x1.aced68254df71p-6,   0x1.a2c8145ab8798p-6,   0x1.98d280b676227p-6,   0x1.8f0c1696b706ep-6,   0x1.85743fd8770eep-6,   0x1.7c0a66dd8735fp-6,   0x1.72cdf6927a98dp-6,   0x1.69be5a7477289p-6,
    0x1.60dafe96e9fe8p-6,   0x1.58234fa91f35cp-6,   0x1.4f96bafbbd3b7p-6,   0x1.4734ae862393cp-6,   0x1.3efc98ebacfd7p-6,   0x1.36ede980d4f0ap-6,   0x1.2f08105040790p-6,   0x1.274a7e1faa59fp-6,
    0x1.1fb4a474b28f9p-6,   0x1.1845f5999118dp-6,   0x1.10fde4a1ac1e4p-6,   0x1.09dbe56e1172cp-6,   0x1.02df6cb1d378ap-6,   0x1.f80fdfec92e59p-7,   0x1.eaa9cb3e66a3ap-7,   0x1.dd8b89dd8210dp-7,
    0x1.d0b40c12faecap-7,   0x1.c42243f439ff3p-7,   0x1.b7d5256a5fbcfp-7,   0x1.abcba639709f8p-7,   0x1.a004be074949ep-7,   0x1.947f66625ac3ap-7,   0x1.893a9ac82ef77p-7,   0x1.7e3558abb5ad1p-7,
    0x1.736e9f7b5a4d0p-7,   0x1.68e570a6e29fep-7,   0x1.5e98cfa516da1p-7,   0x1.5487c1f9333c1p-7,   0x1.4ab14f3823966p-7,   0x1.4114810d88f8dp-7,   0x1.37b0634089f1fp-7,   0x1.2e8403b86da57p-7,
    0x1.258e728102215p-7,   0x1.1ccec1cece44cp-7,   0x1.144406030fa2bp-7,   0x1.0bed55af84c28p-7,   0x1.03c9c99a041cbp-7,   0x1.f7b0f97fc0984p-8,   0x1.e83118b233ac1p-8,   0x1.d9122fb6bdf1fp-8,
    0x1.ca5281f9b3f83p-8,   0x1.bbf0576d9cd3fp-8,   0x1.ade9fc900b6cap-8,   0x1.a03dc26e1787cp-8,   0x1.92e9fea8778d0p-8,   0x1.85ed0b773c08dp-8,   0x1.794547ad2de7bp-8,   0x1.6cf116bad06bdp-8,
    0x1.60eee0b107fa1p-8,   0x1.553d124366b87p-8,   0x1.49da1cca20199p-8,   0x1.3ec47643a473fp-8,   0x1.33fa9955e5ab8p-8,   0x1.297b054f4622bp-8,   0x1.1f443e273304bp-8,   0x1.1554cc7e6b0fbp-8,
    0x1.0bab3d9ef3122p-8,   0x1.0246237bb9356p-8,   0x1.f248295fd0aabp-9,   0x1.e08758fbd90d0p-9,   0x1.cf47159c5416dp-9,   0x1.be84a85acfca2p-9,   0x1.ae3d6395de7ccp-9,   0x1.9e6ea2ecc1d1fp-9,
    0x1.8f15cb3a8454bp-9,   0x1.80304a9084333p-9,   0x1.71bb98307193cp-9,   0x1.63b53485c3178p-9,   0x1.561aa91ea300bp-9,   0x1.48e988a4579aap-9,   0x1.3c1f6ed329625p-9,   0x1.2fba0071c98a6p-9,
    0x1.23b6eb483b663p-9,   0x1.1813e6164354ep-9,   0x1.0cceb0895dc1cp-9,   0x1.01e5133240ceep-9,   0x1.eea9bef3d68ddp-10,  0x1.da37df2c86d79p-10,  0x1.c6704cfc90804p-10,  0x1.b34edfbbafc69p-10,
    0x1.a0cf80121b3a8p-10,  0x1.8eee27de7465ep-10,  0x1.7da6e21af23f3p-10,  0x1.6cf5cac1ca995p-10,  0x1.5cd70eb0dfc14p-10,  0x1.4d46eb8cb77dfp-10,  0x1.3e41afa2c09bdp-10,  0x1.2fc3b9caec33cp-10,
    0x1.21c979489fd3fp-10,  0x1.144f6dab05af9p-10,  0x1.075226acbff2dp-10,  0x1.f59c882608b67p-11,  0x1.dd80eb184a53ep-11,  0x1.c64af51b18f8bp-11,  0x1.aff444625e3fcp-11,  0x1.9a769624f7ba5p-11,
    0x1.85cbc655987ddp-11,  0x1.71edcf5ab7954p-11,  0x1.5ed6c9c595384p-11,  0x1.4c80ec0860958p-11,  0x1.3ae68a2b87fafp-11,  0x1.2a0215823cffcp-11,  0x1.19ce1c5e365e6p-11,  0x1.0a4549c2b8fd2p-11,
    0x1.f6c4ca2de3504p-12,  0x1.da40a3af51c84p-12,  0x1.bef41e90b449dp-12,  0x1.a4d570476f5cap-12,  0x1.8bdb049748823p-12,  0x1.73fb7cf157564p-12,  0x1.5d2dafd206664p-12,  0x1.4768a81e358dcp-12,
    0x1.32a3a47f8f736p-12,  0x1.1ed616c0238a7p-12,  0x1.0bf7a32555d92p-12,  0x1.f4003f946b0abp-13,  0x1.d1cf27f295f64p-13,  0x1.b14c6f0bdc9cfp-13,  0x1.9268e44a6c7ddp-13,  0x1.7515b58d2024bp-13,
    0x1.59446dd82087bp-13,  0x1.3ee6f4051793cp-13,  0x1.25ef897313058p-13,  0x1.0e50c8b636307p-13,  0x1.effb488eb1d5ap-14,  0x1.c5d2ca6762b75p-14,  0x1.9e0f53994bd9cp-14,  0x1.7898c8afc86fcp-14,
    0x1.5557b37fb2795p-14,  0x1.343540894d84ap-14,  0x1.151b3c5b62806p-14,  0x1.efe821ef84d69p-15,  0x1.b9558672c9563p-15,  0x1.8655e078a9c11p-15,  0x1.56c1958f46183p-15,  0x1.2a7231730fb04p-15,
  };

  static constexpr double kbdLongDenominator = 0x1.664cc7e39e450p+8;

  double leftLong[AAC_WINSHAPE_COUNT][AAC_XFORM_HALFWIN_SIZE_LONG];
  double leftStart[AAC_WINSHAPE_COUNT][AAC_XFORM_HALFWIN_SIZE_LONG];  // TODO: Alias to leftLong?
  double leftStop[AAC_WINSHAPE_COUNT][AAC_XFORM_HALFWIN_SIZE_LONG];
  double leftShort[AAC_WINSHAPE_COUNT][AAC_XFORM_HALFWIN_SIZE_SHORT];

  double rightLong[AAC_WINSHAPE_COUNT][AAC_XFORM_HALFWIN_SIZE_LONG];
  double rightStart[AAC_WINSHAPE_COUNT][AAC_XFORM_HALFWIN_SIZE_LONG];
  double rightStop[AAC_WINSHAPE_COUNT][AAC_XFORM_HALFWIN_SIZE_LONG];  // TODO: Alias to rightLong?
  double rightShort[AAC_WINSHAPE_COUNT][AAC_XFORM_HALFWIN_SIZE_SHORT];

  static bool isInitialized = false;

  static unsigned int initKbdLeftShort(double *out)
  {
    for (unsigned int i = 0; i < AAC_XFORM_HALFWIN_SIZE_SHORT; i++)
    {
      double numerator = 0.0;
      for (unsigned int p = 0; p <= i; p++)
        numerator += kbdShort[p];

      *out++ = sqrt(numerator / kbdShortDenominator);
    }

    return AAC_XFORM_HALFWIN_SIZE_SHORT;
  }

  static unsigned int initKbdLeftLong(double *out)
  {
    for (unsigned int i = 0; i < AAC_XFORM_HALFWIN_SIZE_LONG; i++)
    {
      double numerator = 0.0;
      for (unsigned int p = 0; p <= i; p++)
        numerator += kbdLong[p];

      *out++ = sqrt(numerator / kbdLongDenominator);
    }

    return AAC_XFORM_HALFWIN_SIZE_LONG;
  }

  static unsigned int initKbdRightShort(double *out)
  {
    for (unsigned int i = 0; i < AAC_XFORM_HALFWIN_SIZE_SHORT; i++)
    {
      double numerator = 0.0;
      for (unsigned int p = 0; p <= AAC_XFORM_HALFWIN_SIZE_SHORT - 1 - i; p++)
        numerator += kbdShort[p];

      *out++ = sqrt(numerator / kbdShortDenominator);
    }

    return AAC_XFORM_HALFWIN_SIZE_SHORT;
  }

  static unsigned int initKbdRightLong(double *out)
  {
    for (unsigned int i = 0; i < AAC_XFORM_HALFWIN_SIZE_LONG; i++)
    {
      double numerator = 0.0;
      for (unsigned int p = 0; p <= AAC_XFORM_HALFWIN_SIZE_LONG - 1 - i; p++)
        numerator += kbdLong[p];

      *out++ = sqrt(numerator / kbdLongDenominator);
    }

    return AAC_XFORM_HALFWIN_SIZE_LONG;
  }

  // Write samples for the left half of a sine wave hump. Range (0, π / 2).
  static unsigned int initSinLeft(double *out, unsigned int count)
  {
    for (unsigned int i = 0; i < count; i++)
      *out++ = sin((M_PI / (count * 2)) * (i + 0.5));

    return count;
  }

  // Write samples for the right half of a sine wave hump. Range (π / 2, π).
  static unsigned int initSinRight(double *out, unsigned int count)
  {
    for (unsigned int i = 0; i < count; i++)
      *out++ = sin((M_PI / (count * 2)) * (count + i + 0.5));

    return count;
  }

  static unsigned int initCopy(double *out, const double *in, unsigned int count)
  {
    memcpy(out, in, sizeof(double) * count);
    return count;
  }

  static unsigned int initSet(double *out, double v, unsigned int count)
  {
    for (unsigned int i = 0; i < count; i++)
      *out++ = v;

    return count;
  }

  static void initialize(void)
  {
    if (isInitialized) return;

    double *w;

    // --- Left half sine shape

    initSinLeft(leftLong[AAC_WINSHAPE_SIN], AAC_XFORM_HALFWIN_SIZE_LONG);

    initSinLeft(leftShort[AAC_WINSHAPE_SIN], AAC_XFORM_HALFWIN_SIZE_SHORT);

    initCopy(leftStart[AAC_WINSHAPE_SIN], leftLong[AAC_WINSHAPE_SIN], AAC_XFORM_HALFWIN_SIZE_LONG);

    w = leftStop[AAC_WINSHAPE_SIN];
    w += initSet(w, 0.0, 448);
    w += initCopy(w, leftShort[AAC_WINSHAPE_SIN], AAC_XFORM_HALFWIN_SIZE_SHORT);
    w += initSet(w, 1.0, 448);
    assert(w - AAC_XFORM_HALFWIN_SIZE_LONG == leftStop[AAC_WINSHAPE_SIN]);

    // --- Right half sine shape

    initSinRight(rightLong[AAC_WINSHAPE_SIN], AAC_XFORM_HALFWIN_SIZE_LONG);

    initSinRight(rightShort[AAC_WINSHAPE_SIN], AAC_XFORM_HALFWIN_SIZE_SHORT);

    w = rightStart[AAC_WINSHAPE_SIN];
    w += initSet(w, 1.0, 448);
    w += initCopy(w, rightShort[AAC_WINSHAPE_SIN], AAC_XFORM_HALFWIN_SIZE_SHORT);
    w += initSet(w, 0.0, 448);
    assert(w - AAC_XFORM_HALFWIN_SIZE_LONG == rightStart[AAC_WINSHAPE_SIN]);

    initCopy(rightStop[AAC_WINSHAPE_SIN], rightLong[AAC_WINSHAPE_SIN], AAC_XFORM_HALFWIN_SIZE_LONG);

    // --- Left half KBD shape

    initKbdLeftLong(leftLong[AAC_WINSHAPE_KBD]);

    initKbdLeftShort(leftShort[AAC_WINSHAPE_KBD]);

    initCopy(leftStart[AAC_WINSHAPE_KBD], leftLong[AAC_WINSHAPE_KBD], AAC_XFORM_HALFWIN_SIZE_LONG);

    w = leftStop[AAC_WINSHAPE_KBD];
    w += initSet(w, 0.0, 448);
    w += initCopy(w, leftShort[AAC_WINSHAPE_KBD], AAC_XFORM_HALFWIN_SIZE_SHORT);
    w += initSet(w, 1.0, 448);
    assert(w - AAC_XFORM_HALFWIN_SIZE_LONG == leftStop[AAC_WINSHAPE_KBD]);

    // --- Right half KBD shape

    initKbdRightLong(rightLong[AAC_WINSHAPE_KBD]);

    initKbdRightShort(rightShort[AAC_WINSHAPE_KBD]);

    w = rightStart[AAC_WINSHAPE_KBD];
    w += initSet(w, 1.0, 448);
    w += initCopy(w, rightShort[AAC_WINSHAPE_KBD], AAC_XFORM_HALFWIN_SIZE_SHORT);
    w += initSet(w, 0.0, 448);
    assert(w - AAC_XFORM_HALFWIN_SIZE_LONG == rightStart[AAC_WINSHAPE_KBD]);

    initCopy(rightStop[AAC_WINSHAPE_KBD], rightLong[AAC_WINSHAPE_KBD], AAC_XFORM_HALFWIN_SIZE_LONG);

    // All done
    isInitialized = true;
  }

  const double *getLeftWindow(AacWindowShape shape, AacWindowSequence sequence)
  {
    if (!isInitialized)
      initialize();

    switch (sequence)
    {
    case AAC_WINSEQ_LONG:
      return leftLong[shape];
    case AAC_WINSEQ_8_SHORT:
      return leftShort[shape];
    case AAC_WINSEQ_LONG_START:
      return leftStart[shape];
    case AAC_WINSEQ_LONG_STOP:
      return leftStop[shape];
    }

    abort();  // Not reached
  }

  const double *getRightWindow(AacWindowShape shape, AacWindowSequence sequence)
  {
    if (!isInitialized)
      initialize();

    switch (sequence)
    {
    case AAC_WINSEQ_LONG:
      return rightLong[shape];
    case AAC_WINSEQ_8_SHORT:
      return rightShort[shape];
    case AAC_WINSEQ_LONG_START:
      return rightStart[shape];
    case AAC_WINSEQ_LONG_STOP:
      return rightStop[shape];
    }

    abort();  // Not reached
  }

};
